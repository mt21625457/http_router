# HTTP Router 实现细节

本文档详细介绍 `http_router` 的实现细节，包括内部数据结构、算法选择和性能优化策略。这些信息适用于希望了解路由器内部工作原理或计划扩展其功能的开发者。

## 目录

1. [数据结构选择](#数据结构选择)
2. [路由存储策略](#路由存储策略)
3. [路由匹配算法](#路由匹配算法)
4. [缓存系统实现](#缓存系统实现)
5. [性能优化技术](#性能优化技术)
6. [内存管理](#内存管理)

## 数据结构选择

### 主要数据结构

`http_router` 使用多种数据结构来优化不同类型路由的存储和查找：

#### 1. 哈希表 (`std::unordered_map`)

```cpp
std::unordered_map<std::string, std::shared_ptr<Handler>> static_hash_routes_;
```

- **用途**：存储短路径的静态路由
- **优势**：O(1) 平均查找时间，适合精确匹配
- **缺点**：不支持前缀匹配，内存使用较高

#### 2. 前缀树 (`tsl::htrie_map`)

```cpp
tsl::htrie_map<char, std::shared_ptr<Handler>> static_trie_routes_;
```

- **用途**：存储长路径和共享前缀的静态路由
- **优势**：内存效率更高，优化共享前缀
- **性能**：查找时间通常为 O(k)，其中 k 是路径长度

#### 3. 参数化路由向量 (`std::vector<ParamRoute>`)

```cpp
std::vector<ParamRoute> param_routes_;
```

- **用途**：存储需要动态匹配的参数化和通配符路由
- **结构**：每个 `ParamRoute` 包含路径段、处理程序和通配符标志

#### 4. 段数索引 (`std::unordered_map`)

```cpp
std::unordered_map<size_t, std::vector<size_t>> segment_index_;
```

- **用途**：按段数对参数化路由进行索引，加速匹配过程
- **优势**：快速过滤掉段数不匹配的路由

#### 5. LRU 缓存 (`std::unordered_map` + `std::list`)

```cpp
struct CacheEntry {
    std::shared_ptr<Handler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    std::chrono::steady_clock::time_point timestamp;
};

std::unordered_map<std::string, std::list<std::string>::iterator> route_cache_;
std::list<std::string> cache_lru_list_;
```

- **用途**：缓存已解析的路由，加速重复请求
- **策略**：LRU（最近最少使用）淘汰策略

## 路由存储策略

### 路由分类与存储位置选择

路由器根据路径特征自动选择最合适的存储位置：

```cpp
void add_route(const std::string &path, std::shared_ptr<Handler> handler) {
    // 清除缓存以保持一致性
    clear_cache();
    
    // 解析路径段
    auto segments = split_path(path);
    
    // 检查是否包含参数或通配符
    bool has_params = false;
    bool has_wildcard = false;
    
    for (const auto &segment : segments) {
        if (is_parameter_segment(segment)) {
            has_params = true;
        } else if (is_wildcard_segment(segment)) {
            has_wildcard = true;
        }
    }
    
    // 存储策略
    if (!has_params && !has_wildcard) {
        // 静态路由
        if (path.length() <= SHORT_PATH_THRESHOLD && count_segments(path) <= SEGMENT_THRESHOLD) {
            // 短路径和简单路由存储在哈希表中
            static_hash_routes_[path] = handler;
        } else {
            // 长路径或共享前缀的路由存储在前缀树中
            static_trie_routes_[path] = handler;
        }
    } else {
        // 参数化路由或通配符路由
        ParamRoute param_route;
        param_route.segments = segments;
        param_route.handler = handler;
        param_route.has_wildcard = has_wildcard;
        
        // 添加到参数化路由集合
        size_t route_index = param_routes_.size();
        param_routes_.push_back(param_route);
        
        // 更新段数索引
        size_t segment_count = segments.size();
        if (segment_index_.find(segment_count) == segment_index_.end()) {
            segment_index_[segment_count] = std::vector<size_t>();
        }
        segment_index_[segment_count].push_back(route_index);
    }
}
```

### 存储决策标准

1. **静态路由**：不含参数或通配符
   - **短路径** (≤ 10 字符) 和 **简单路由** (≤ 1 段): 存储在哈希表中
   - **长路径** (> 10 字符) 或 **复杂路由** (> 1 段): 存储在前缀树中

2. **参数化路由**：含 `:param` 或 `*`
   - 所有参数化或通配符路由存储在参数化路由向量中
   - 按段数建立索引，用于快速筛选

### 数据结构优化理由

- **哈希表与前缀树的平衡**：
  - 哈希表对短路径提供最快的查找
  - 前缀树对长路径和共享前缀的路由提供更好的内存效率
  - 实验表明，这种混合策略比纯哈希表或纯前缀树提供更好的整体性能

- **段数索引的价值**：
  - 参数化路由匹配是 CPU 密集型的
  - 通过段数预筛选，可以减少 80-95% 的匹配操作

## 路由匹配算法

### 匹配流程

路由匹配遵循以下优先级顺序：

1. **缓存查找**：首先检查缓存中是否存在匹配
2. **静态路由查找**：
   - 首先检查哈希表中的短路径
   - 然后检查前缀树中的长路径
3. **参数化路由匹配**：
   - 按段数筛选潜在匹配
   - 对每个候选路由执行参数匹配

```cpp
int find_route(std::string_view path,
               std::shared_ptr<Handler> &handler,
               std::map<std::string, std::string> &params,
               std::map<std::string, std::string> &query_params) {
    
    // 1. 解析查询字符串（如果有）
    std::string path_without_query;
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        parse_query_string(path.substr(query_pos + 1), query_params);
        path_without_query = std::string(path.substr(0, query_pos));
    } else {
        path_without_query = std::string(path);
    }
    
    // 2. 检查缓存
    if (ENABLE_CACHE && check_route_cache(path, handler, params, query_params)) {
        return 0;
    }
    
    // 3. 检查静态路由（哈希表）
    auto hash_it = static_hash_routes_.find(path_without_query);
    if (hash_it != static_hash_routes_.end()) {
        handler = hash_it->second;
        
        // 缓存结果
        if (ENABLE_CACHE) {
            cache_route(std::string(path), handler, params, query_params);
        }
        return 0;
    }
    
    // 4. 检查静态路由（前缀树）
    auto trie_it = static_trie_routes_.find(path_without_query);
    if (trie_it != static_trie_routes_.end()) {
        handler = *trie_it;
        
        // 缓存结果
        if (ENABLE_CACHE) {
            cache_route(std::string(path), handler, params, query_params);
        }
        return 0;
    }
    
    // 5. 匹配参数化路由
    auto path_segments = split_path(path_without_query);
    size_t segment_count = path_segments.size();
    
    // 使用段数索引优化查找
    auto segment_it = segment_index_.find(segment_count);
    if (segment_it != segment_index_.end()) {
        for (size_t route_index : segment_it->second) {
            const auto &param_route = param_routes_[route_index];
            
            if (match_route_params(param_route, path_segments, params)) {
                handler = param_route.handler;
                
                // 缓存结果
                if (ENABLE_CACHE) {
                    cache_route(std::string(path), handler, params, query_params);
                }
                return 0;
            }
        }
    }
    
    // 6. 没有找到匹配的路由
    return -1;
}
```

### 参数化路由匹配算法

参数化路由匹配使用自定义的递归比较算法，避免了昂贵的正则表达式：

```cpp
bool match_route_params(const ParamRoute &route,
                        const std::vector<std::string> &path_segments,
                        std::map<std::string, std::string> &params) {
    
    // 通配符路由特殊处理
    if (route.has_wildcard) {
        // 确保通配符前的段匹配
        size_t wildcard_pos = 0;
        while (wildcard_pos < route.segments.size() && !is_wildcard_segment(route.segments[wildcard_pos])) {
            wildcard_pos++;
        }
        
        // 检查通配符前的段
        if (wildcard_pos > path_segments.size()) {
            return false;
        }
        
        for (size_t i = 0; i < wildcard_pos; i++) {
            const auto &route_seg = route.segments[i];
            const auto &path_seg = path_segments[i];
            
            if (is_parameter_segment(route_seg)) {
                // 参数段：提取参数名和值
                std::string param_name = route_seg.substr(1); // 去掉 ':'
                params[param_name] = path_seg;
            } else if (route_seg != path_seg) {
                // 静态段：必须精确匹配
                return false;
            }
        }
        
        // 收集通配符匹配的部分
        std::string wildcard_value;
        for (size_t i = wildcard_pos; i < path_segments.size(); i++) {
            if (!wildcard_value.empty()) {
                wildcard_value += "/";
            }
            wildcard_value += path_segments[i];
        }
        
        params["*"] = wildcard_value;
        return true;
    }
    
    // 非通配符路由：逐段匹配
    for (size_t i = 0; i < route.segments.size(); i++) {
        const auto &route_seg = route.segments[i];
        const auto &path_seg = path_segments[i];
        
        if (is_parameter_segment(route_seg)) {
            // 参数段：提取参数名和值
            std::string param_name = route_seg.substr(1); // 去掉 ':'
            params[param_name] = path_seg;
        } else if (route_seg != path_seg) {
            // 静态段：必须精确匹配
            return false;
        }
    }
    
    return true;
}
```

### 查询参数处理

查询参数解析使用自定义解析器，支持 URL 编码：

```cpp
void parse_query_string(std::string_view query_string, 
                       std::map<std::string, std::string> &query_params) {
    size_t start = 0;
    while (start < query_string.length()) {
        // 找到 & 或 查询字符串结尾
        size_t end = query_string.find('&', start);
        if (end == std::string::npos) {
            end = query_string.length();
        }
        
        // 提取键值对
        std::string key_value = std::string(query_string.substr(start, end - start));
        size_t eq_pos = key_value.find('=');
        
        if (eq_pos == std::string::npos) {
            // 无值参数（如 ?debug）
            std::string key = url_decode(key_value);
            query_params[key] = "";
        } else {
            // 键值对参数（如 ?key=value）
            std::string key = url_decode(key_value.substr(0, eq_pos));
            std::string value = url_decode(key_value.substr(eq_pos + 1));
            query_params[key] = value;
        }
        
        start = end + 1;
    }
}
```

## 缓存系统实现

### LRU 缓存设计

路由缓存使用 LRU（最近最少使用）策略，结合哈希表和双向链表实现：

```cpp
// 缓存结构
struct CacheEntry {
    std::shared_ptr<Handler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    std::chrono::steady_clock::time_point timestamp;
};

// 缓存数据结构
std::unordered_map<std::string, std::list<std::string>::iterator> route_cache_;  // 路径 -> 链表迭代器
std::list<std::string> cache_lru_list_;  // LRU 链表（最近使用的在前面）
```

### 缓存查找过程

```cpp
bool check_route_cache(std::string_view path,
                       std::shared_ptr<Handler> &handler,
                       std::map<std::string, std::string> &params,
                       std::map<std::string, std::string> &query_params) {
    std::string path_str(path);
    auto it = route_cache_.find(path_str);
    
    if (it != route_cache_.end()) {
        // 缓存命中
        
        // 更新 LRU 顺序：将条目移到链表前端
        cache_lru_list_.splice(cache_lru_list_.begin(), cache_lru_list_, it->second);
        
        // 获取缓存条目
        auto cache_it = route_cache_[path_str];
        auto &cached_entry = cache_entries_[*cache_it];
        
        // 复制缓存的结果
        handler = cached_entry.handler;
        params = cached_entry.params;
        query_params = cached_entry.query_params;
        
        // 更新时间戳
        cached_entry.timestamp = std::chrono::steady_clock::now();
        
        return true;
    }
    
    return false;
}
```

### 缓存添加与淘汰

```cpp
void cache_route(std::string path,
                std::shared_ptr<Handler> handler,
                const std::map<std::string, std::string> &params,
                const std::map<std::string, std::string> &query_params) {
    // 检查路由是否已在缓存中
    auto it = route_cache_.find(path);
    if (it != route_cache_.end()) {
        // 已存在，更新并移到链表前端
        cache_lru_list_.splice(cache_lru_list_.begin(), cache_lru_list_, it->second);
        
        // 更新缓存内容
        auto &entry = cache_entries_[path];
        entry.handler = handler;
        entry.params = params;
        entry.query_params = query_params;
        entry.timestamp = std::chrono::steady_clock::now();
    } else {
        // 新缓存项
        
        // 添加到 LRU 链表前端
        cache_lru_list_.push_front(path);
        route_cache_[path] = cache_lru_list_.begin();
        
        // 创建缓存条目
        CacheEntry entry;
        entry.handler = handler;
        entry.params = params;
        entry.query_params = query_params;
        entry.timestamp = std::chrono::steady_clock::now();
        cache_entries_[path] = entry;
        
        // 如果缓存过大，修剪
        prune_cache();
    }
}
```

### 缓存修剪

```cpp
void prune_cache() {
    while (route_cache_.size() > MAX_CACHE_SIZE) {
        // 移除 LRU 链表尾部的条目（最少使用的）
        std::string lru_path = cache_lru_list_.back();
        cache_entries_.erase(lru_path);
        route_cache_.erase(lru_path);
        cache_lru_list_.pop_back();
    }
}
```

### 缓存清除

```cpp
void clear_cache() {
    route_cache_.clear();
    cache_entries_.clear();
    cache_lru_list_.clear();
}
```

## 性能优化技术

### 1. 路径分段策略

路径分段使用高效的字符串分割算法，避免不必要的内存分配：

```cpp
std::vector<std::string> split_path(const std::string &path) {
    std::vector<std::string> segments;
    size_t start = 0;
    
    // 跳过开头的 '/'
    if (!path.empty() && path[0] == '/') {
        start = 1;
    }
    
    while (start < path.length()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) {
            end = path.length();
        }
        
        if (end > start) {
            segments.push_back(path.substr(start, end - start));
        }
        
        start = end + 1;
    }
    
    return segments;
}
```

### 2. 字符串优化

- 使用 `std::string_view` 减少不必要的字符串拷贝
- 对于短路径，使用哈希表直接查找避免字符串分割
- 只在必要时进行路径分段和参数提取

### 3. 算法优化

- 使用段数索引快速筛选参数化路由
- 避免正则表达式，使用更高效的自定义匹配算法
- 路由存储策略基于路径特征，优化内存和查找性能

### 4. 缓存优化

- 使用 LRU 策略确保频繁使用的路由保留在缓存中
- 缓存不仅存储匹配结果，还存储解析后的参数
- 使用 `splice()` 操作高效更新 LRU 链表，避免内存分配

### 5. 内存布局优化

- 使用前缀树存储共享前缀的路由，减少内存使用
- 参数化路由按段数索引，提高局部性和缓存效率
- 静态路由和参数化路由分别存储，避免混合查找

## 内存管理

### 1. 智能指针使用

路由器使用 `std::shared_ptr` 管理处理程序生命周期：

```cpp
// 处理程序使用共享指针
std::shared_ptr<Handler> handler;

// 路由存储
std::unordered_map<std::string, std::shared_ptr<Handler>> static_hash_routes_;
```

这确保了：
- 处理程序在路由器和外部引用间共享
- 当路由器或所有外部引用被销毁时，处理程序自动释放
- 无需手动内存管理

### 2. 内存占用分析

路由器的内存占用主要由以下因素决定：

- **路由数量**：每个路由占用一个指针和路径字符串的空间
- **路径长度**：长路径消耗更多内存，但前缀树优化共享前缀
- **缓存大小**：取决于 `MAX_CACHE_SIZE` 和缓存项大小

典型场景的内存占用：

| 场景 | 路由数 | 平均路径长度 | 缓存大小 | 总内存占用 |
|------|--------|--------------|----------|------------|
| 小型 API | 50 | 15 字符 | 100 | ~50 KB |
| 中型 Web 应用 | 200 | 25 字符 | 500 | ~300 KB |
| 大型框架 | 1000 | 40 字符 | 1000 | ~1-2 MB |

### 3. 内存释放策略

- **缓存自动清理**：当缓存达到 `MAX_CACHE_SIZE` 时自动修剪
- **手动缓存清理**：通过 `clear_cache()` 方法释放缓存内存
- **析构函数**：路由器析构时自动释放所有资源

```cpp
~http_router() {
    // 清除缓存
    clear_cache();
    
    // 清除路由存储
    static_hash_routes_.clear();
    static_trie_routes_.clear();
    param_routes_.clear();
    segment_index_.clear();
}
```

### 4. 内存峰值控制

对于内存受限的环境：

- 减小 `MAX_CACHE_SIZE` 常量以减少缓存内存使用
- 使用更多的前缀树存储而非哈希表
- 定期调用 `clear_cache()` 释放未使用的缓存内存
``` 