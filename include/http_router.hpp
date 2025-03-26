/**
 * @file http_router.hpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tsl/htrie_map.h"

template <typename Handler>
class http_router
{
private:
    struct RouteInfo
    {
        std::shared_ptr<Handler> handler;
        std::vector<std::string> param_names;
        bool has_wildcard;

        RouteInfo() : has_wildcard(false) {}
    };

    // 路由缓存条目
    struct CacheEntry
    {
        std::shared_ptr<Handler> handler;
        std::map<std::string, std::string> params;

        // 用于LRU淘汰策略的时间戳
        std::chrono::steady_clock::time_point last_accessed;

        CacheEntry() : last_accessed(std::chrono::steady_clock::now()) {}

        void update_timestamp() { last_accessed = std::chrono::steady_clock::now(); }
    };

    // 混合数据结构策略
    // 1. 使用哈希表存储短路径和随机路径 (O(1)查找)
    std::unordered_map<std::string, RouteInfo> static_hash_routes_;

    // 2. 使用htrie_map存储有共同前缀的长路径 (前缀树优势)
    tsl::htrie_map<char, RouteInfo> static_trie_routes_;

    // 3. 参数化路由仍使用向量存储
    std::vector<std::pair<std::string, RouteInfo>> param_routes_;

    // 路径段数索引，用于加速参数化路由匹配
    std::unordered_map<size_t, std::vector<size_t>> segment_index_;

    // 路由缓存，提供O(1)路由查找
    mutable std::unordered_map<std::string, CacheEntry> route_cache_;

    // 路由缓存LRU列表，用于管理缓存容量
    mutable std::list<std::string> cache_lru_list_;

    // 路径类型判断阈值
    static constexpr size_t SHORT_PATH_THRESHOLD = 10; // 短路径的最大长度
    static constexpr size_t SEGMENT_THRESHOLD = 1;     // 路径段阈值

    // 缓存配置
    static constexpr size_t MAX_CACHE_SIZE = 1000; // 最大缓存条目数
    static constexpr bool ENABLE_CACHE = true;     // 是否启用缓存

public:
    http_router() = default;
    ~http_router() = default;

public:
    /**
     * @brief Add a route to the router
     * @brief 添加路由到路由器
     *
     * @param path          The URL path to add (e.g. "/users/:id")
     *                      要添加的URL路径 (例如 "/users/:id")
     *
     * @param handler       The handler to be called when the route matches
     *                      当路由匹配时调用的处理程序
     */
    void add_route(const std::string &path, std::shared_ptr<Handler> handler)
    {
        // Validate inputs
        if (path.empty() || !handler) {
            return;
        }

        // 创建路由信息
        RouteInfo route_info;
        route_info.handler = handler;

        // Check if the path contains parameters or wildcards
        if (path.find(':') != std::string::npos || path.find('*') != std::string::npos) {
            // Parse parameter names and check for wildcards
            std::string pattern_path = path;
            size_t pos = 0;

            // Find and store parameter names
            while ((pos = pattern_path.find(':', pos)) != std::string::npos) {
                size_t end = pattern_path.find('/', pos);
                if (end == std::string::npos) {
                    end = pattern_path.length();
                }

                std::string param_name = pattern_path.substr(pos + 1, end - pos - 1);
                route_info.param_names.push_back(param_name);
                pos = end;
            }

            // Check for wildcards
            if (pattern_path.find('*') != std::string::npos) {
                route_info.has_wildcard = true;
                route_info.param_names.push_back("*");
            }

            // Store and index parameterized route
            size_t index = param_routes_.size();
            param_routes_.emplace_back(path, route_info);

            // 计算路径段数并索引
            std::vector<std::string> segments;
            split_path(path, segments);
            segment_index_[segments.size()].push_back(index);
        } else {
            // 静态路由 - 智能选择存储结构
            size_t segment_count = count_segments(path);

            // 根据路径特征选择合适的数据结构
            if (path.length() <= SHORT_PATH_THRESHOLD || segment_count <= SEGMENT_THRESHOLD) {
                // 短路径或段数少，使用哈希表 (O(1)查找)
                static_hash_routes_[path] = route_info;
            } else {
                // 长路径且段数多，使用Trie树 (利用前缀共享优势)
                static_trie_routes_.insert(path, route_info);
            }
        }

        // 路由更新时清除缓存，确保路由映射的一致性
        clear_cache();
    }

    /**
     * @brief Find route by matching the given path against registered routes
     * @brief 根据给定的路径查找匹配的路由
     *
     * @param path          The URL path to match (e.g. "/users/123?name=test")
     *                      要匹配的URL路径 (例如 "/users/123?name=test")
     *
     * @param handler       Output parameter that will be set to the matched route's handler
     *                      输出参数,将被设置为匹配路由的处理程序
     *
     * @param params        Output map that will be filled with path parameters (e.g. {id: "123"}
     * for "/users/:id") 输出参数映射,将被填充路径参数 (例如 "/users/:id" 路径会得到 {id: "123"})
     *
     * @param query_params  Output map that will be filled with query parameters (e.g. {name:
     * "test"} for "?name=test") 输出参数映射,将被填充查询参数 (例如 "?name=test" 会得到 {name:
     * "test"})
     *
     * @return int         0 on successful match, -1 if no route matches
     *                     匹配成功返回0,没有匹配的路由返回-1
     */
    int find_route(std::string_view path, std::shared_ptr<Handler> &handler,
                   std::map<std::string, std::string> &params,
                   std::map<std::string, std::string> &query_params)
    {
        if (path.empty()) {
            return -1;
        }

        // Extract query parameters if present
        std::string_view path_without_query = path;
        auto query_pos = path.find('?');
        if (query_pos != std::string_view::npos) {
            path_without_query = path.substr(0, query_pos);
            parse_query_params(path.substr(query_pos + 1), query_params);
        }

        // 转换为标准字符串，用于查找
        std::string path_str(path_without_query);

        // 检查缓存是否有匹配路由，提供O(1)查找
        if (ENABLE_CACHE) {
            if (check_route_cache(std::string(path), handler, params)) {
                return 0;
            }
        }

        // 1. 首先检查哈希表 (最快 O(1))
        auto hash_it = static_hash_routes_.find(path_str);
        if (hash_it != static_hash_routes_.end()) {
            handler = hash_it->second.handler;

            // 更新路由缓存
            if (ENABLE_CACHE) {
                cache_route(std::string(path), handler, params);
            }
            return 0;
        }

        // 2. 然后检查前缀树 (适合共享前缀的路径)
        auto trie_it = static_trie_routes_.find(path_str);
        if (trie_it != static_trie_routes_.end()) {
            handler = trie_it.value().handler;

            // 更新路由缓存
            if (ENABLE_CACHE) {
                cache_route(std::string(path), handler, params);
            }
            return 0;
        }

        // 3. 最后尝试参数化路由匹配
        // 优化：先按段数筛选，减少匹配次数
        std::vector<std::string> path_segments;
        split_path(path_str, path_segments);

        // 首先尝试匹配相同段数的路由
        auto segment_it = segment_index_.find(path_segments.size());
        if (segment_it != segment_index_.end()) {
            for (size_t idx : segment_it->second) {
                const auto &route_pair = param_routes_[idx];
                if (match_route(path_str, route_pair.first, route_pair.second, params)) {
                    handler = route_pair.second.handler;

                    // 更新路由缓存
                    if (ENABLE_CACHE) {
                        cache_route(std::string(path), handler, params);
                    }
                    return 0;
                }
            }
        }

        // 尝试匹配通配符路由（可能有不同段数）
        for (const auto &route_pair : param_routes_) {
            // 跳过已检查过的相同段数路由
            if (segment_it != segment_index_.end() && !route_pair.second.has_wildcard) {
                continue;
            }

            if (route_pair.second.has_wildcard &&
                match_route(path_str, route_pair.first, route_pair.second, params)) {
                handler = route_pair.second.handler;

                // 更新路由缓存
                if (ENABLE_CACHE) {
                    cache_route(std::string(path), handler, params);
                }
                return 0;
            }
        }

        return -1;
    }

    // 清除路由缓存
    void clear_cache()
    {
        route_cache_.clear();
        cache_lru_list_.clear();
    }

private:
    // 检查路由缓存并返回匹配结果
    bool check_route_cache(const std::string &path, std::shared_ptr<Handler> &handler,
                           std::map<std::string, std::string> &params) const
    {
        auto it = route_cache_.find(path);
        if (it != route_cache_.end()) {
            // 命中缓存，直接返回结果
            handler = it->second.handler;
            params = it->second.params;

            // 更新访问时间戳（LRU策略）
            it->second.update_timestamp();

            // 将路径移到LRU列表前端
            auto lru_it = std::find(cache_lru_list_.begin(), cache_lru_list_.end(), path);
            if (lru_it != cache_lru_list_.end()) {
                cache_lru_list_.erase(lru_it);
                cache_lru_list_.push_front(path);
            }

            return true;
        }
        return false;
    }

    // 将路由结果缓存
    void cache_route(const std::string &path, const std::shared_ptr<Handler> &handler,
                     const std::map<std::string, std::string> &params) const
    {
        // 检查路径是否已经在缓存中
        auto it = route_cache_.find(path);
        if (it != route_cache_.end()) {
            // 更新现有缓存条目
            it->second.handler = handler;
            it->second.params = params;
            it->second.update_timestamp();

            // 移动到LRU列表前端
            auto lru_it = std::find(cache_lru_list_.begin(), cache_lru_list_.end(), path);
            if (lru_it != cache_lru_list_.end()) {
                cache_lru_list_.erase(lru_it);
                cache_lru_list_.push_front(path);
            } else {
                // 如果找不到就添加到前端（应该不会发生，但为安全起见）
                cache_lru_list_.push_front(path);
            }
            return;
        }

        // 如果缓存已满，清除最久未使用的条目
        if (route_cache_.size() >= MAX_CACHE_SIZE) {
            prune_cache();
        }

        // 添加到缓存
        CacheEntry entry;
        entry.handler = handler;
        entry.params = params;
        entry.update_timestamp();

        route_cache_[path] = entry;
        cache_lru_list_.push_front(path);
    }

    // 清理最久未使用的缓存条目
    void prune_cache() const
    {
        // 基于LRU列表删除
        while (!cache_lru_list_.empty() && route_cache_.size() >= MAX_CACHE_SIZE) {
            // 移除LRU列表中的尾部元素（最久未使用）
            std::string lru_path = cache_lru_list_.back();
            cache_lru_list_.pop_back();
            route_cache_.erase(lru_path);
        }
    }

    // 计算路径段数
    size_t count_segments(const std::string &path) const
    {
        size_t count = 0;
        for (char c : path) {
            if (c == '/') {
                count++;
            }
        }
        return count > 0 ? count : 1;
    }

    // Custom route matching function that doesn't use regex
    bool match_route(const std::string &path, const std::string &pattern,
                     const RouteInfo &route_info, std::map<std::string, std::string> &params)
    {
        std::vector<std::string> path_segments;
        std::vector<std::string> pattern_segments;

        // Split path and pattern into segments
        split_path(path, path_segments);
        split_path(pattern, pattern_segments);

        // Check if wildcard pattern
        if (route_info.has_wildcard) {
            // Find where the wildcard is
            size_t wildcard_idx = 0;
            for (; wildcard_idx < pattern_segments.size(); ++wildcard_idx) {
                if (pattern_segments[wildcard_idx] == "*") {
                    break;
                }
            }

            // If wildcard is at the end, just match all preceding segments
            if (wildcard_idx == pattern_segments.size() - 1) {
                if (path_segments.size() < wildcard_idx) {
                    return false;
                }

                // Match all segments before wildcard
                for (size_t i = 0; i < wildcard_idx; ++i) {
                    if (!match_segment(path_segments[i], pattern_segments[i], params)) {
                        return false;
                    }
                }

                // Capture the rest as wildcard
                std::string wildcard_value;
                for (size_t i = wildcard_idx; i < path_segments.size(); ++i) {
                    if (i > wildcard_idx) {
                        wildcard_value += "/";
                    }
                    wildcard_value += path_segments[i];
                }
                params["*"] = wildcard_value;
                return true;
            } else {
                // More complex wildcard handling in the middle - not implemented
                return false;
            }
        } else if (path_segments.size() != pattern_segments.size()) {
            // Without wildcards, segment counts must match
            return false;
        } else {
            // Match each segment
            for (size_t i = 0; i < path_segments.size(); ++i) {
                if (!match_segment(path_segments[i], pattern_segments[i], params)) {
                    return false;
                }
            }
            return true;
        }
    }

    // Match a single segment, handling parameters
    bool match_segment(const std::string &path_segment, const std::string &pattern_segment,
                       std::map<std::string, std::string> &params)
    {
        // If pattern segment starts with ':', it's a parameter
        if (!pattern_segment.empty() && pattern_segment[0] == ':') {
            std::string param_name = pattern_segment.substr(1);
            params[param_name] = path_segment;
            return true;
        }

        // Otherwise, direct comparison
        return path_segment == pattern_segment;
    }

    // Split a path into segments
    void split_path(const std::string &path, std::vector<std::string> &segments)
    {
        segments.clear();

        size_t start = 0;
        if (path[0] == '/') {
            start = 1;
        }

        size_t pos;
        while ((pos = path.find('/', start)) != std::string::npos) {
            if (pos > start) {
                segments.push_back(path.substr(start, pos - start));
            }
            start = pos + 1;
        }

        if (start < path.length()) {
            segments.push_back(path.substr(start));
        }
    }

    // Parse query parameters from URL (e.g., "key1=value1&key2=value2")
    void parse_query_params(std::string_view query, std::map<std::string, std::string> &params)
    {
        size_t start = 0;
        size_t end;

        while (start < query.length()) {
            // Find parameter separator
            end = query.find('&', start);
            if (end == std::string_view::npos) {
                end = query.length();
            }

            // Find key-value separator
            size_t equal_pos = query.find('=', start);
            if (equal_pos != std::string_view::npos && equal_pos < end) {
                std::string key = std::string(query.substr(start, equal_pos - start));
                std::string value = std::string(query.substr(equal_pos + 1, end - equal_pos - 1));

                // URL decode key and value
                url_decode(key);
                url_decode(value);

                // Store parameter
                params[key] = value;
            } else {
                // Key with no value
                std::string key = std::string(query.substr(start, end - start));
                url_decode(key);
                params[key] = "";
            }

            start = end + 1;
        }
    }

    // URL decode a string (convert %xx to characters and + to space)
    void url_decode(std::string &str)
    {
        std::string result;
        result.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%' && i + 2 < str.size()) {
                // Handle percent-encoded characters
                int value = 0;
                if (hex_to_int(str[i + 1], value)) {
                    value *= 16;
                    int second_value = 0;
                    if (hex_to_int(str[i + 2], second_value)) {
                        value += second_value;
                        result.push_back(static_cast<char>(value));
                        i += 2;
                        continue;
                    }
                }
            } else if (str[i] == '+') {
                // Convert + to space
                result.push_back(' ');
                continue;
            }

            // Pass through other characters
            result.push_back(str[i]);
        }

        str = result;
    }

    // Convert hex character to integer
    bool hex_to_int(char c, int &value)
    {
        if (c >= '0' && c <= '9') {
            value = c - '0';
            return true;
        } else if (c >= 'a' && c <= 'f') {
            value = c - 'a' + 10;
            return true;
        } else if (c >= 'A' && c <= 'F') {
            value = c - 'A' + 10;
            return true;
        }
        return false;
    }
};
