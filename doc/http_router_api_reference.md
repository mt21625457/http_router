# HTTP Router API 参考

## 类与模板

### `http_router<Handler>`

```cpp
template <typename Handler>
class http_router;
```

主路由器类，负责存储路由和匹配传入的请求路径。

#### 模板参数

- `Handler`: 与路由关联的处理程序类型。可以是任何类型，通常是一个函数对象或类，用于处理匹配到路由时的操作。

---

## 公共成员函数

### 构造函数与析构函数

#### `http_router()`

```cpp
http_router();
```

默认构造函数，创建一个空的路由器实例。

#### `~http_router()`

```cpp
~http_router();
```

析构函数，释放路由器分配的所有资源。

---

### 路由管理

#### `add_route`

```cpp
void add_route(const std::string &path, std::shared_ptr<Handler> handler);
```

向路由器添加一个新路由。

**参数**:
- `path`: 要匹配的路径模式。可以包含静态段、参数段（以`:`开头）和通配符（`*`）。
- `handler`: 与路由关联的处理程序，当路径匹配成功时将被调用。

**路径语法**:
- 静态路径: `/about`, `/products`
- 参数化路径: `/users/:id`, `/api/:version/resources/:resourceId`
- 通配符路径: `/static/*`, `/files/:type/*`

**注意**:
- 通配符只能出现在路径的末尾
- 参数名必须是有效的标识符（字母、数字和下划线）
- 路径必须以`/`开头

**示例**:
```cpp
router.add_route("/", std::make_shared<HomeHandler>());
router.add_route("/users/:id", std::make_shared<UserHandler>());
router.add_route("/files/*", std::make_shared<FileHandler>());
```

---

#### `find_route`

```cpp
int find_route(std::string_view path, 
               std::shared_ptr<Handler> &handler,
               std::map<std::string, std::string> &params,
               std::map<std::string, std::string> &query_params);
```

查找匹配给定路径的路由。

**参数**:
- `path`: 要匹配的路径，可以包含查询参数部分（`?key=value`）
- `handler`: 输出参数，成功匹配时会设置为关联的处理程序
- `params`: 输出参数，用于存储从路径中提取的参数值
- `query_params`: 输出参数，用于存储从查询字符串中解析的参数

**返回值**:
- `0`: 成功找到匹配的路由
- `-1`: 没有找到匹配的路由

**注意**:
- 路由匹配遵循以下优先级：缓存 > 静态哈希表 > 静态前缀树 > 参数化路由
- 如果找到匹配的路由，`handler`、`params` 和 `query_params` 将被设置
- 查询参数会从路径中提取并解析，支持 URL 编码的值

**示例**:
```cpp
std::shared_ptr<Handler> handler;
std::map<std::string, std::string> params;
std::map<std::string, std::string> query_params;

if (router.find_route("/users/123?active=true&role=admin", handler, params, query_params) == 0) {
    // 匹配成功，handler 设置为与 "/users/:id" 关联的处理程序
    // params = {{"id", "123"}}
    // query_params = {{"active", "true"}, {"role", "admin"}}
}
```

---

#### `clear_cache`

```cpp
void clear_cache();
```

清除路由器的路由缓存。

**用途**:
- 当路由配置更改时调用此方法以确保一致性
- 在内存受限的环境中手动管理缓存大小

**示例**:
```cpp
// 添加新路由后清除缓存
router.add_route("/new/path", std::make_shared<NewHandler>());
router.clear_cache();
```

---

## 私有成员函数

### 路径处理

#### `is_parameter_segment`

```cpp
bool is_parameter_segment(const std::string &segment);
```

检查路径段是否是参数段（以`:`开头）。

---

#### `is_wildcard_segment`

```cpp
bool is_wildcard_segment(const std::string &segment);
```

检查路径段是否是通配符段（`*`）。

---

#### `split_path`

```cpp
std::vector<std::string> split_path(const std::string &path);
```

将路径拆分为段的向量。

---

#### `parse_query_string`

```cpp
void parse_query_string(std::string_view query_string, 
                        std::map<std::string, std::string> &query_params);
```

解析查询字符串并填充查询参数映射。

---

#### `url_decode`

```cpp
std::string url_decode(const std::string &encoded);
```

解码 URL 编码的字符串。

---

### 缓存管理

#### `check_route_cache`

```cpp
bool check_route_cache(std::string_view path,
                      std::shared_ptr<Handler> &handler,
                      std::map<std::string, std::string> &params,
                      std::map<std::string, std::string> &query_params);
```

检查缓存中是否存在匹配的路由。

---

#### `cache_route`

```cpp
void cache_route(std::string path,
                 std::shared_ptr<Handler> handler,
                 const std::map<std::string, std::string> &params,
                 const std::map<std::string, std::string> &query_params);
```

将路由结果存储在缓存中。

---

#### `prune_cache`

```cpp
void prune_cache();
```

如果缓存大小超过最大值，则修剪缓存。

---

### 路由匹配

#### `match_route_params`

```cpp
bool match_route_params(const ParamRoute &route,
                        const std::vector<std::string> &path_segments,
                        std::map<std::string, std::string> &params);
```

尝试将参数化路由与给定的路径段匹配。

---

#### `count_segments`

```cpp
size_t count_segments(const std::string &path);
```

计算路径中的段数。

---

## 私有成员变量

### 路由存储

```cpp
std::unordered_map<std::string, std::shared_ptr<Handler>> static_hash_routes_;
```
存储短路径的哈希表路由集合。

```cpp
tsl::htrie_map<char, std::shared_ptr<Handler>> static_trie_routes_;
```
存储长路径和共享前缀的前缀树路由集合。

```cpp
struct ParamRoute {
    std::vector<std::string> segments;  // 路径段
    std::shared_ptr<Handler> handler;   // 处理程序
    bool has_wildcard;                  // 是否包含通配符
};

std::vector<ParamRoute> param_routes_;
```
存储参数化路由和通配符路由。

```cpp
std::unordered_map<size_t, std::vector<size_t>> segment_index_;
```
按段数索引参数化路由，用于快速查找。

### 缓存系统

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
LRU 缓存存储系统，用于快速路由查找。

---

## 常量和配置参数

```cpp
static constexpr size_t SHORT_PATH_THRESHOLD = 10;
```
决定路径是应存储在哈希表中还是前缀树中的长度阈值。

```cpp
static constexpr size_t SEGMENT_THRESHOLD = 1;
```
决定何时使用段数索引优化的段数阈值。

```cpp
static constexpr size_t MAX_CACHE_SIZE = 1000;
```
路由缓存的最大项数。

```cpp
static constexpr bool ENABLE_CACHE = true;
```
控制是否启用路由缓存的标志。

---

## 辅助类型

### `ParamRoute`

```cpp
struct ParamRoute {
    std::vector<std::string> segments;  // 路径段
    std::shared_ptr<Handler> handler;   // 处理程序
    bool has_wildcard;                  // 是否包含通配符
};
```

存储参数化路由信息的结构体。

### `CacheEntry`

```cpp
struct CacheEntry {
    std::shared_ptr<Handler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    std::chrono::steady_clock::time_point timestamp;
};
```

存储缓存路由信息的结构体，包括处理程序、参数和时间戳。

---

## 错误处理

路由器中的错误处理主要通过返回值进行：

- 路由不存在: `find_route` 返回 `-1`
- 内部错误: 通过日志记录或抛出异常处理

路由器不会抛出异常，除非:
- 内存分配失败
- 底层容器（如 tsl::htrie_map）抛出异常

---

## 线程安全性

`http_router` 类本身不是线程安全的。对于多线程应用程序：

- **读取并发**：允许多个线程同时调用 `find_route` 方法。
- **写入互斥**：在调用 `add_route` 或 `clear_cache` 时，应使用互斥锁保护路由器实例。

推荐的同步模式：

```cpp
std::shared_mutex router_mutex;
http_router<Handler> router;

// 读取（路由查找）
{
    std::shared_lock<std::shared_mutex> lock(router_mutex);
    router.find_route(...);
}

// 写入（添加路由或清除缓存）
{
    std::unique_lock<std::shared_mutex> lock(router_mutex);
    router.add_route(...);
    // 或
    router.clear_cache();
}
```

---

## 性能考虑

### 时间复杂度

| 操作 | 时间复杂度 | 备注 |
|-----|-----------|------|
| 添加静态路由 | O(1) | 哈希表插入 |
| 添加参数化路由 | O(n) | n 为路径段数 |
| 查找静态路由 | O(1) | 哈希表查找 |
| 查找参数化路由 | O(m * n) | m 为参数化路由数，n 为路径段数 |
| 缓存查找 | O(1) | 哈希表查找 |
| 清除缓存 | O(n) | n 为缓存项数 |

### 内存复杂度

- 静态路由: O(n) - n 为路由数
- 参数化路由: O(n * m) - n 为路由数，m 为每个路由的平均段数
- 缓存: O(c) - c 为缓存大小（最大 MAX_CACHE_SIZE）

---

## 使用示例

### 基本路由设置

```cpp
#include "http_router.hpp"
#include <memory>

// 创建路由器
http_router<std::function<void(const std::map<std::string, std::string>&)>> router;

// 添加静态路由
router.add_route("/", std::make_shared<std::function<void(const std::map<std::string, std::string>&)>>(
    [](const std::map<std::string, std::string>& params) {
        // 处理首页请求
    }
));

// 添加参数化路由
router.add_route("/users/:id", std::make_shared<std::function<void(const std::map<std::string, std::string>&)>>(
    [](const std::map<std::string, std::string>& params) {
        std::string user_id = params.at("id");
        // 处理用户请求
    }
));

// 添加通配符路由
router.add_route("/files/*", std::make_shared<std::function<void(const std::map<std::string, std::string>&)>>(
    [](const std::map<std::string, std::string>& params) {
        std::string wildcard = params.at("*");
        // 处理文件请求
    }
));
```

### 路由查找

```cpp
// 处理传入请求
void handle_request(const std::string& path) {
    std::shared_ptr<std::function<void(const std::map<std::string, std::string>&)>> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    if (router.find_route(path, handler, params, query_params) == 0) {
        // 调用处理程序
        (*handler)(params);
        
        // 使用查询参数
        for (const auto& [key, value] : query_params) {
            // 处理查询参数
        }
    } else {
        // 处理 404 未找到
    }
}
```

### 自定义处理程序类

```cpp
class RequestHandler {
public:
    RequestHandler(const std::string& name) : name_(name) {}
    
    void handle(const std::map<std::string, std::string>& params,
                const std::map<std::string, std::string>& query_params) {
        // 处理请求
    }
    
private:
    std::string name_;
};

// 创建路由器
http_router<RequestHandler> router;

// 添加路由
router.add_route("/api/users", std::make_shared<RequestHandler>("ListUsers"));
router.add_route("/api/users/:id", std::make_shared<RequestHandler>("GetUser"));

// 查找路由
std::shared_ptr<RequestHandler> handler;
std::map<std::string, std::string> params;
std::map<std::string, std::string> query_params;

if (router.find_route("/api/users/123", handler, params, query_params) == 0) {
    handler->handle(params, query_params);
}
```

---

## 最佳实践

### 路由注册

- 按照从最具体到最不具体的顺序注册路由
- 将常用路由放在前面可提高匹配性能
- 注册路由后应用缓存

### 路由查找

- 重用 `params` 和 `query_params` 映射以减少分配
- 避免在性能关键代码中频繁调用 `clear_cache`
- 考虑使用 `std::string_view` 作为路径参数以减少复制

### 线程安全

- 在多线程环境中使用读写锁保护路由器
- 路由注册应在应用程序启动时完成，而不是运行时
- 缓存清理操作应在低流量时执行

---

## 常见问题解答

**Q: 通配符和参数化路由的区别是什么？**

A: 参数化路由（如 `/users/:id`）会匹配特定段，而通配符路由（如 `/files/*`）会匹配剩余的所有段。通配符更灵活但优先级更低。

**Q: 如何处理路由冲突？**

A: 路由器采用以下优先级：静态路由 > 参数化路由 > 通配符路由。如果有冲突，将选择较高优先级的路由。

**Q: 路由缓存如何影响性能？**

A: 对于重复访问的路径，缓存可提供 5-20 倍的性能提升。缓存特别适用于高流量、稳定路由的应用。

**Q: 路由器支持正则表达式匹配吗？**

A: 不支持。为了优化性能，路由器使用自定义匹配算法，避免正则表达式的开销。

**Q: 缓存如何处理过期或无效的路由？**

A: 当添加新路由或调用 `clear_cache` 时，缓存会被清除。缓存还使用 LRU（最近最少使用）策略自动管理容量。 