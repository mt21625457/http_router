/**
 * @file router.hpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief High-performance HTTP router with caching and advanced routing
 * capabilities 高性能HTTP路由器，支持缓存和高级路由功能
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once

#include <algorithm> // Required for std::transform
#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tsl/htrie_map.h"

namespace flc {

// Forward declaration for router_group
// router_group类的前向声明
template <typename Handler>
class router_group;

/**
 * @brief HTTP method enumeration
 *        HTTP方法枚举
 *
 * Supports all standard HTTP methods plus UNKNOWN for error handling
 * 支持所有标准HTTP方法，外加用于错误处理的UNKNOWN
 */
enum class HttpMethod
{
    GET,     ///< HTTP GET method - 获取资源
    POST,    ///< HTTP POST method - 创建资源
    PUT,     ///< HTTP PUT method - 更新资源
    DELETE,  ///< HTTP DELETE method - 删除资源
    PATCH,   ///< HTTP PATCH method - 部分更新资源
    HEAD,    ///< HTTP HEAD method - 获取资源头信息
    OPTIONS, ///< HTTP OPTIONS method - 获取支持的方法
    CONNECT, ///< HTTP CONNECT method - 建立隧道连接
    TRACE,   ///< HTTP TRACE method - 回显请求
    UNKNOWN  ///< Unknown or unsupported method - 未知或不支持的方法
};

/**
 * @brief Convert HttpMethod enum to string representation
 *        将HttpMethod枚举转换为字符串表示
 *
 * @param method The HTTP method to convert / 要转换的HTTP方法
 * @return std::string String representation of the method / 方法的字符串表示
 *
 * @example
 * ```cpp
 * std::string method_str = to_string(HttpMethod::GET); // Returns "GET"
 * ```
 */
inline std::string to_string(HttpMethod method)
{
    switch (method) {
    case HttpMethod::GET:
        return "GET";
    case HttpMethod::POST:
        return "POST";
    case HttpMethod::PUT:
        return "PUT";
    case HttpMethod::DELETE:
        return "DELETE";
    case HttpMethod::PATCH:
        return "PATCH";
    case HttpMethod::HEAD:
        return "HEAD";
    case HttpMethod::OPTIONS:
        return "OPTIONS";
    case HttpMethod::CONNECT:
        return "CONNECT";
    case HttpMethod::TRACE:
        return "TRACE";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Convert string to HttpMethod enum (case-insensitive)
 *        将字符串转换为HttpMethod枚举（不区分大小写）
 *
 * @param s The string to convert / 要转换的字符串
 * @return HttpMethod The corresponding HTTP method / 对应的HTTP方法
 *
 * @example
 * ```cpp
 * HttpMethod method = from_string("get"); // Returns HttpMethod::GET
 * HttpMethod method2 = from_string("POST"); // Returns HttpMethod::POST
 * ```
 */
inline HttpMethod from_string(std::string_view s)
{
    std::string upper_s;
    upper_s.reserve(s.length());
    std::transform(s.begin(), s.end(), std::back_inserter(upper_s), ::toupper);

    if (upper_s == "GET")
        return HttpMethod::GET;
    if (upper_s == "POST")
        return HttpMethod::POST;
    if (upper_s == "PUT")
        return HttpMethod::PUT;
    if (upper_s == "DELETE")
        return HttpMethod::DELETE;
    if (upper_s == "PATCH")
        return HttpMethod::PATCH;
    if (upper_s == "HEAD")
        return HttpMethod::HEAD;
    if (upper_s == "OPTIONS")
        return HttpMethod::OPTIONS;
    if (upper_s == "CONNECT")
        return HttpMethod::CONNECT;
    if (upper_s == "TRACE")
        return HttpMethod::TRACE;
    return HttpMethod::UNKNOWN;
}

/**
 * @brief High-performance HTTP router with advanced features
 *        高性能HTTP路由器，具有高级功能
 *
 * @tparam Handler The type of handler function used for route callbacks
 *                 用于路由回调的处理器函数类型
 *
 * Features / 功能特性:
 * - Thread-safe routing with thread-local caching / 线程安全路由与线程本地缓存
 * - Multiple storage strategies for optimal performance /
 * 多种存储策略实现最优性能
 * - Support for static, parameterized, and wildcard routes /
 * 支持静态、参数化和通配符路由
 * - LRU cache for frequently accessed routes / LRU缓存用于频繁访问的路由
 * - Query parameter parsing with URL decoding / 查询参数解析与URL解码
 * - Router groups for organizing routes / 路由组用于组织路由
 *
 * @example
 * ```cpp
 * // Create a router
 * router<MyHandler> r;
 *
 * // Add routes
 * r.add_route(HttpMethod::GET, "/users/:id", handler);
 * r.add_route(HttpMethod::POST, "/users", create_handler);
 * r.add_route(HttpMethod::GET, "/static/\\*", static_handler); // Wildcard route
 *
 * // Find and execute route
 * std::shared_ptr<MyHandler> handler;
 * std::map<std::string, std::string> params, query_params;
 * if (r.find_route(HttpMethod::GET, "/users/123?name=john", handler, params,
 * query_params) == 0) {
 *     // Route found, params["id"] == "123", query_params["name"] == "john"
 *     handler->handle();
 * }
 * ```
 */
template <typename Handler>
class router
{
private:
    /**
     * @brief Route information storage structure
     *        路由信息存储结构
     */
    struct RouteInfo
    {
        std::shared_ptr<Handler> handler;     ///< Route handler / 路由处理器
        std::vector<std::string> param_names; ///< Parameter names for parameterized
                                              ///< routes / 参数化路由的参数名
        bool has_wildcard;                    ///< Whether route has wildcard / 路由是否包含通配符

        RouteInfo() : has_wildcard(false) {}
    };

    /**
     * @brief Cache entry for storing route lookup results
     *        用于存储路由查找结果的缓存条目
     */
    struct CacheEntry
    {
        std::shared_ptr<Handler> handler;          ///< Cached handler / 缓存的处理器
        std::map<std::string, std::string> params; ///< Cached parameters / 缓存的参数
        std::list<std::string>::iterator lru_it;   ///< LRU list iterator for O(1) updates /
                                                   ///< LRU列表迭代器用于O(1)更新
    };

    // Storage structures optimized for different route patterns
    // 针对不同路由模式优化的存储结构

    /**
     * @brief Hash map storage for short static routes (O(1) lookup)
     *        短静态路由的哈希表存储（O(1)查找）
     */
    std::unordered_map<HttpMethod, std::unordered_map<std::string, RouteInfo>>
        static_hash_routes_by_method_;

    /**
     * @brief Trie storage for long static routes with common prefixes
     *        具有公共前缀的长静态路由的字典树存储
     */
    std::unordered_map<HttpMethod, tsl::htrie_map<char, RouteInfo>> static_trie_routes_by_method_;

    /**
     * @brief Vector storage for parameterized routes
     *        参数化路由的向量存储
     */
    std::unordered_map<HttpMethod, std::vector<std::pair<std::string, RouteInfo>>>
        param_routes_by_method_;

    /**
     * @brief Segment count index for optimizing parameterized route matching
     *        段数索引用于优化参数化路由匹配
     */
    std::unordered_map<HttpMethod, std::unordered_map<size_t, std::vector<size_t>>>
        segment_index_by_method_;

    /**
     * @brief Thread-local cache access methods for route lookup results
     *        线程本地缓存访问方法用于路由查找结果
     */
    static std::unordered_map<std::string, CacheEntry> &get_thread_local_cache()
    {
        thread_local std::unordered_map<std::string, CacheEntry> cache;
        return cache;
    }

    static std::list<std::string> &get_thread_local_lru_list()
    {
        thread_local std::list<std::string> lru_list;
        return lru_list;
    }

    // Performance tuning constants
    // 性能调优常量
    static constexpr size_t SHORT_PATH_THRESHOLD =
        10; ///< Maximum length for short paths / 短路径的最大长度
    static constexpr size_t SEGMENT_THRESHOLD = 1; ///< Path segment threshold / 路径段阈值
    static constexpr size_t MAX_CACHE_SIZE =
        5000; ///< Maximum cache entries per thread / 每个线程的最大缓存条目数
    static constexpr bool ENABLE_CACHE = true; ///< Whether to enable caching / 是否启用缓存

public:
    /**
     * @brief Default constructor
     *        默认构造函数
     */
    router() = default;

    /**
     * @brief Default destructor
     *        默认析构函数
     */
    ~router() = default;

public:
    /**
     * @brief Add a route to the router with intelligent storage selection
     *        向路由器添加路由，智能选择存储方式
     *
     * @param method HTTP method for this route / 此路由的HTTP方法
     * @param path URL path pattern (supports :param and * wildcards) /
     * URL路径模式（支持:param和*通配符）
     * @param handler Shared pointer to the route handler / 路由处理器的共享指针
     *
     * Path patterns / 路径模式:
     * - Static: "/users", "/api/health" / 静态: "/users", "/api/health"
     * - Parameterized: "/users/:id", "/api/:version/users/:userId" / 参数化:
     * "/users/:id", "/api/:version/users/:userId"
     * - Wildcard: "/static/\\*", "/files/:type/\\*" / 通配符: "/static/\\*",
     * "/files/:type/\\*"
     *
     * @example
     * ```cpp
     * router<MyHandler> r;
     *
     * // Static route
     * r.add_route(HttpMethod::GET, "/health", health_handler);
     *
     * // Parameterized route
     * r.add_route(HttpMethod::GET, "/users/:id", user_handler);
     *
     * // Wildcard route
     * r.add_route(HttpMethod::GET, "/static/\\*", static_file_handler);
     * ```
     */
    void add_route(HttpMethod method, const std::string &path, std::shared_ptr<Handler> handler);

    /**
     * @brief Find route by matching path and extract parameters
     *        通过匹配路径查找路由并提取参数
     *
     * @param method HTTP method to match / 要匹配的HTTP方法
     * @param path Full URL path with optional query string /
     * 完整的URL路径，可选查询字符串
     * @param handler [OUT] Matched route handler / [输出] 匹配的路由处理器
     * @param params [OUT] Path parameters extracted from URL / [输出]
     * 从URL提取的路径参数
     * @param query_params [OUT] Query parameters from URL / [输出]
     * URL中的查询参数
     *
     * @return int 0 on successful match, -1 if no route matches /
     * 匹配成功返回0，无匹配路由返回-1
     *
     * @example
     * ```cpp
     * std::shared_ptr<MyHandler> handler;
     * std::map<std::string, std::string> params, query_params;
     *
     * // Match: /users/123?sort=name&order=asc
     * int result = r.find_route(HttpMethod::GET,
     * "/users/123?sort=name&order=asc", handler, params, query_params);
     *
     * if (result == 0) {
     *     // params["id"] == "123"
     *     // query_params["sort"] == "name", query_params["order"] == "asc"
     *     handler->handle();
     * }
     * ```
     */
    int find_route(HttpMethod method, std::string_view path, std::shared_ptr<Handler> &handler,
                   std::map<std::string, std::string> &params,
                   std::map<std::string, std::string> &query_params);

    /**
     * @brief Clear thread-local route cache
     *        清除线程本地路由缓存
     *
     * This only clears the cache for the current thread. Other threads' caches
     * remain intact. 这仅清除当前线程的缓存。其他线程的缓存保持不变。
     */
    void clear_cache();

    /**
     * @brief Create a router group with optional prefix
     *        创建带有可选前缀的路由组
     *
     * @param prefix Path prefix for the group (e.g., "/api", "/v1") /
     * 组的路径前缀（例如"/api", "/v1"）
     * @return std::shared_ptr<router_group<Handler>> Shared pointer to the
     * created router group / 创建的路由组的共享指针
     *
     * @example
     * ```cpp
     * auto api_group = r.group("/api");
     * auto v1_group = api_group->group("/v1");
     *
     * // Routes will be prefixed automatically
     * v1_group->get("/users", handler); // Matches "/api/v1/users"
     * ```
     */
    std::shared_ptr<router_group<Handler>> group(std::string_view prefix = "");

private:
    /**
     * @brief Normalize path by removing redundant slashes and trailing slashes
     *        通过删除多余斜杠和尾部斜杠来规范化路径
     *
     * @param path [IN/OUT] Path to normalize / [输入/输出] 要规范化的路径
     */
    static void normalize_path(std::string &path);

    /**
     * @brief Generate cache key for route lookup
     *        为路由查找生成缓存键
     *
     * @param method HTTP method / HTTP方法
     * @param path URL path / URL路径
     * @return std::string Cache key / 缓存键
     */
    std::string generate_cache_key(HttpMethod method, const std::string &path) const;

    /**
     * @brief Check thread-local route cache for matching route
     *        检查线程本地路由缓存中的匹配路由
     *
     * @param method HTTP method / HTTP方法
     * @param path URL path / URL路径
     * @param handler [OUT] Cached handler if found / [输出] 找到的缓存处理器
     * @param params [OUT] Cached parameters if found / [输出] 找到的缓存参数
     * @return bool True if cache hit, false otherwise /
     * 缓存命中返回true，否则返回false
     */
    bool check_route_cache(HttpMethod method, const std::string &path,
                           std::shared_ptr<Handler> &handler,
                           std::map<std::string, std::string> &params) const;

    /**
     * @brief Cache route lookup result in thread-local cache
     *        在线程本地缓存中缓存路由查找结果
     *
     * @param method HTTP method / HTTP方法
     * @param path URL path / URL路径
     * @param handler Route handler to cache / 要缓存的路由处理器
     * @param params Route parameters to cache / 要缓存的路由参数
     */
    void cache_route(HttpMethod method, const std::string &path,
                     const std::shared_ptr<Handler> &handler,
                     const std::map<std::string, std::string> &params) const;

    /**
     * @brief Remove least recently used entries from thread-local cache
     *        从线程本地缓存中删除最近最少使用的条目
     */
    void prune_cache() const;

    /**
     * @brief Count the number of path segments
     *        计算路径段数
     *
     * @param path URL path / URL路径
     * @return size_t Number of segments / 段数
     */
    size_t count_segments(const std::string &path) const;

    /**
     * @brief Custom route matching without regex (high performance)
     *        不使用正则表达式的自定义路由匹配（高性能）
     *
     * @param path Request path / 请求路径
     * @param pattern Route pattern / 路由模式
     * @param route_info Route information / 路由信息
     * @param params [OUT] Extracted parameters / [输出] 提取的参数
     * @return bool True if match, false otherwise / 匹配返回true，否则返回false
     */
    bool match_route(const std::string &path, const std::string &pattern,
                     const RouteInfo &route_info, std::map<std::string, std::string> &params);

    /**
     * @brief Match a single path segment against pattern segment
     *        将单个路径段与模式段匹配
     *
     * @param path_segment Path segment from request / 请求中的路径段
     * @param pattern_segment Pattern segment (may contain :param) /
     * 模式段（可能包含:param）
     * @param params [OUT] Parameters extracted / [输出] 提取的参数
     * @return bool True if match, false otherwise / 匹配返回true，否则返回false
     */
    bool match_segment(const std::string &path_segment, const std::string &pattern_segment,
                       std::map<std::string, std::string> &params);

    /**
     * @brief Split path into segments for matching
     *        将路径分割为段以进行匹配
     *
     * @param path URL path / URL路径
     * @param segments [OUT] Path segments / [输出] 路径段
     */
    void split_path(const std::string &path, std::vector<std::string> &segments);

    /**
     * @brief Parse query parameters from URL with URL decoding
     *        从URL解析查询参数并进行URL解码
     *
     * @param query Query string (without leading ?) / 查询字符串（不含前导?）
     * @param params [OUT] Parsed parameters / [输出] 解析的参数
     */
    void parse_query_params(std::string_view query, std::map<std::string, std::string> &params);

    /**
     * @brief URL decode string (convert %xx to characters and + to space)
     *        URL解码字符串（将%xx转换为字符，+转换为空格）
     *
     * @param str [IN/OUT] String to decode / [输入/输出] 要解码的字符串
     */
    void url_decode(std::string &str);

    /**
     * @brief Convert hexadecimal character to integer
     *        将十六进制字符转换为整数
     *
     * @param c Hex character / 十六进制字符
     * @param value [OUT] Integer value / [输出] 整数值
     * @return bool True if valid hex, false otherwise /
     * 有效十六进制返回true，否则返回false
     */
    bool hex_to_int(char c, int &value);
};

// ========== TEMPLATE METHOD IMPLEMENTATIONS ==========

template <typename Handler>
void router<Handler>::add_route(HttpMethod method, const std::string &path,
                                std::shared_ptr<Handler> handler)
{
    std::string normalized_path = path;
    normalize_path(normalized_path);

    // Clear cache to maintain consistency
    if (ENABLE_CACHE) {
        clear_cache();
    }

    // Parse path information
    std::vector<std::string> segments;
    split_path(normalized_path, segments);

    bool has_params = false;
    bool has_wildcard = false;
    std::vector<std::string> param_names;

    for (size_t i = 0; i < segments.size(); i++) {
        const auto &segment = segments[i];
        if (!segment.empty() && segment[0] == ':') {
            has_params = true;
            param_names.push_back(segment.substr(1));
        } else if (segment == "*") {
            // Wildcard can only be at the end
            if (i == segments.size() - 1) {
                has_wildcard = true;
            }
            // No need to process further segments after wildcard
            break;
        }
    }

    // Create route info
    RouteInfo route_info;
    route_info.handler = handler;
    route_info.param_names = param_names;
    route_info.has_wildcard = has_wildcard;

    // Storage strategy based on route characteristics
    if (!has_params && !has_wildcard) {
        // Static route
        if (normalized_path.length() <= SHORT_PATH_THRESHOLD &&
            segments.size() <= SEGMENT_THRESHOLD) {
            // Short static route - use hash map
            static_hash_routes_by_method_[method][normalized_path] = route_info;
        } else {
            // Long static route - use trie
            static_trie_routes_by_method_[method][normalized_path] = route_info;
        }
    } else {
        // Parameterized or wildcard route
        size_t route_index = param_routes_by_method_[method].size();
        param_routes_by_method_[method].push_back({normalized_path, route_info});

        // Update segment index for faster lookup
        size_t segment_count = segments.size();
        segment_index_by_method_[method][segment_count].push_back(route_index);
    }
}

template <typename Handler>
int router<Handler>::find_route(HttpMethod method, std::string_view path,
                                std::shared_ptr<Handler> &handler,
                                std::map<std::string, std::string> &params,
                                std::map<std::string, std::string> &query_params)
{
    params.clear();
    query_params.clear();

    // Parse query parameters
    std::string path_without_query;
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        parse_query_params(path.substr(query_pos + 1), query_params);
        path_without_query = std::string(path.substr(0, query_pos));
    } else {
        path_without_query = std::string(path);
    }

    normalize_path(path_without_query);

    // Check cache first
    if (ENABLE_CACHE && check_route_cache(method, path_without_query, handler, params)) {
        return 0;
    }

    // Try static routes - hash map first
    auto hash_method_it = static_hash_routes_by_method_.find(method);
    if (hash_method_it != static_hash_routes_by_method_.end()) {
        auto route_it = hash_method_it->second.find(path_without_query);
        if (route_it != hash_method_it->second.end()) {
            handler = route_it->second.handler;
            if (ENABLE_CACHE) {
                cache_route(method, path_without_query, handler, params);
            }
            return 0;
        }
    }

    // Try static routes - trie
    auto trie_method_it = static_trie_routes_by_method_.find(method);
    if (trie_method_it != static_trie_routes_by_method_.end()) {
        auto trie_it = trie_method_it->second.find(path_without_query);
        if (trie_it != trie_method_it->second.end()) {
            handler = trie_it.value().handler;
            if (ENABLE_CACHE) {
                cache_route(method, path_without_query, handler, params);
            }
            return 0;
        }
    }

    // Try parameterized routes
    auto param_method_it = param_routes_by_method_.find(method);
    if (param_method_it != param_routes_by_method_.end()) {
        std::vector<std::string> path_segments;
        split_path(path_without_query, path_segments);

        // Try to use segment index optimization first
        bool found_via_index = false;
        auto segment_method_it = segment_index_by_method_.find(method);
        if (segment_method_it != segment_index_by_method_.end()) {
            auto segment_it = segment_method_it->second.find(path_segments.size());
            if (segment_it != segment_method_it->second.end()) {
                for (size_t route_index : segment_it->second) {
                    const auto &route_pair = param_method_it->second[route_index];
                    if (match_route(path_without_query, route_pair.first, route_pair.second,
                                    params)) {
                        handler = route_pair.second.handler;
                        if (ENABLE_CACHE) {
                            cache_route(method, path_without_query, handler, params);
                        }
                        return 0;
                    }
                }
                found_via_index = true;
            }
        }

        // If segment index didn't help, try all parameterized routes
        if (!found_via_index) {
            for (const auto &route_pair : param_method_it->second) {
                if (match_route(path_without_query, route_pair.first, route_pair.second, params)) {
                    handler = route_pair.second.handler;
                    if (ENABLE_CACHE) {
                        cache_route(method, path_without_query, handler, params);
                    }
                    return 0;
                }
            }
        }
    }

    return -1; // Route not found
}

template <typename Handler>
void router<Handler>::clear_cache()
{
    auto &cache = get_thread_local_cache();
    auto &lru_list = get_thread_local_lru_list();
    cache.clear();
    lru_list.clear();
}

template <typename Handler>
void router<Handler>::normalize_path(std::string &path)
{
    if (path.empty()) {
        path = "/";
        return;
    }

    std::string result;
    result.reserve(path.length());

    // Ensure path starts with '/'
    if (path[0] != '/') {
        result += '/';
    }

    // Process path character by character, collapsing consecutive slashes
    bool last_was_slash = false;
    for (char c : path) {
        if (c == '/') {
            if (!last_was_slash) {
                result += c;
                last_was_slash = true;
            }
        } else {
            result += c;
            last_was_slash = false;
        }
    }

    // Remove trailing slash except for root path
    if (result.length() > 1 && result.back() == '/') {
        result.pop_back();
    }

    path = result;
}

template <typename Handler>
std::string router<Handler>::generate_cache_key(HttpMethod method, const std::string &path) const
{
    return to_string(method) + ":" + path;
}

template <typename Handler>
bool router<Handler>::check_route_cache(HttpMethod method, const std::string &path,
                                        std::shared_ptr<Handler> &handler,
                                        std::map<std::string, std::string> &params) const
{
    auto &cache = get_thread_local_cache();
    auto &lru_list = get_thread_local_lru_list();

    std::string cache_key = generate_cache_key(method, path);
    auto it = cache.find(cache_key);

    if (it != cache.end()) {
        // Cache hit - move to front of LRU list
        lru_list.splice(lru_list.begin(), lru_list, it->second.lru_it);

        handler = it->second.handler;
        params = it->second.params;
        return true;
    }

    return false;
}

template <typename Handler>
void router<Handler>::cache_route(HttpMethod method, const std::string &path,
                                  const std::shared_ptr<Handler> &handler,
                                  const std::map<std::string, std::string> &params) const
{
    auto &cache = get_thread_local_cache();
    auto &lru_list = get_thread_local_lru_list();

    std::string cache_key = generate_cache_key(method, path);

    // Check if already in cache
    auto it = cache.find(cache_key);
    if (it != cache.end()) {
        // Update existing entry and move to front
        lru_list.splice(lru_list.begin(), lru_list, it->second.lru_it);
        it->second.handler = handler;
        it->second.params = params;
        return;
    }

    // Add new entry
    lru_list.push_front(cache_key);

    CacheEntry entry;
    entry.handler = handler;
    entry.params = params;
    entry.lru_it = lru_list.begin();

    cache[cache_key] = entry;

    // Prune if necessary
    prune_cache();
}

template <typename Handler>
void router<Handler>::prune_cache() const
{
    auto &cache = get_thread_local_cache();
    auto &lru_list = get_thread_local_lru_list();

    while (cache.size() > MAX_CACHE_SIZE) {
        std::string lru_key = lru_list.back();
        cache.erase(lru_key);
        lru_list.pop_back();
    }
}

template <typename Handler>
size_t router<Handler>::count_segments(const std::string &path) const
{
    if (path.empty() || path == "/") {
        return 0;
    }

    size_t count = 0;
    for (char c : path) {
        if (c == '/') {
            count++;
        }
    }

    return count;
}

template <typename Handler>
bool router<Handler>::match_route(const std::string &path, const std::string &pattern,
                                  const RouteInfo &route_info,
                                  std::map<std::string, std::string> &params)
{
    std::vector<std::string> path_segments, pattern_segments;
    split_path(path, path_segments);
    split_path(pattern, pattern_segments);

    if (route_info.has_wildcard) {
        // Handle wildcard routes - wildcard must be the last segment
        if (path_segments.size() < pattern_segments.size() - 1) {
            return false;
        }

        // Verify the last pattern segment is wildcard
        if (pattern_segments.empty() || pattern_segments.back() != "*") {
            return false;
        }

        // Match segments before wildcard
        size_t param_index = 0;
        size_t wildcard_pos = pattern_segments.size() - 1;

        for (size_t i = 0; i < wildcard_pos; i++) {
            const auto &pattern_seg = pattern_segments[i];

            if (i >= path_segments.size()) {
                return false;
            }

            if (!pattern_seg.empty() && pattern_seg[0] == ':') {
                // Parameter segment
                if (param_index < route_info.param_names.size()) {
                    params[route_info.param_names[param_index++]] = path_segments[i];
                }
            } else {
                // Static segment
                if (pattern_seg != path_segments[i]) {
                    return false;
                }
            }
        }

        // Collect remaining path segments as wildcard
        std::string wildcard_value;
        for (size_t j = wildcard_pos; j < path_segments.size(); j++) {
            if (!wildcard_value.empty()) {
                wildcard_value += "/";
            }
            wildcard_value += path_segments[j];
        }
        params["*"] = wildcard_value;
        return true;
    } else {
        // Non-wildcard parameterized route
        if (path_segments.size() != pattern_segments.size()) {
            return false;
        }

        size_t param_index = 0;
        for (size_t i = 0; i < pattern_segments.size(); i++) {
            const auto &pattern_seg = pattern_segments[i];
            const auto &path_seg = path_segments[i];

            if (pattern_seg == "*") {
                // Wildcard in non-wildcard route - this should not happen
                // but if it does, reject the match
                return false;
            } else if (!pattern_seg.empty() && pattern_seg[0] == ':') {
                // Parameter segment
                if (param_index < route_info.param_names.size()) {
                    params[route_info.param_names[param_index++]] = path_seg;
                }
            } else {
                // Static segment
                if (pattern_seg != path_seg) {
                    return false;
                }
            }
        }

        return true;
    }
}

template <typename Handler>
bool router<Handler>::match_segment(const std::string &path_segment,
                                    const std::string &pattern_segment,
                                    std::map<std::string, std::string> &params)
{
    if (pattern_segment[0] == ':') {
        // Parameter segment
        std::string param_name = pattern_segment.substr(1);
        params[param_name] = path_segment;
        return true;
    } else if (pattern_segment == "*") {
        // Wildcard
        params["*"] = path_segment;
        return true;
    } else {
        // Static segment
        return pattern_segment == path_segment;
    }
}

template <typename Handler>
void router<Handler>::split_path(const std::string &path, std::vector<std::string> &segments)
{
    segments.clear();

    if (path.empty() || path == "/") {
        return;
    }

    size_t start = 0;
    if (path[0] == '/') {
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
}

template <typename Handler>
void router<Handler>::parse_query_params(std::string_view query,
                                         std::map<std::string, std::string> &params)
{
    size_t start = 0;
    while (start < query.length()) {
        size_t end = query.find('&', start);
        if (end == std::string::npos) {
            end = query.length();
        }

        std::string key_value = std::string(query.substr(start, end - start));
        size_t eq_pos = key_value.find('=');

        if (eq_pos == std::string::npos) {
            // No value parameter
            url_decode(key_value);
            params[key_value] = "";
        } else {
            std::string key = key_value.substr(0, eq_pos);
            std::string value = key_value.substr(eq_pos + 1);
            url_decode(key);
            url_decode(value);
            params[key] = value;
        }

        start = end + 1;
    }
}

template <typename Handler>
void router<Handler>::url_decode(std::string &str)
{
    std::string result;
    result.reserve(str.length());

    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < str.length()) {
            int value1, value2;
            if (hex_to_int(str[i + 1], value1) && hex_to_int(str[i + 2], value2)) {
                result += static_cast<char>(value1 * 16 + value2);
                i += 2;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }

    str = result;
}

template <typename Handler>
bool router<Handler>::hex_to_int(char c, int &value)
{
    if (c >= '0' && c <= '9') {
        value = c - '0';
        return true;
    } else if (c >= 'A' && c <= 'F') {
        value = c - 'A' + 10;
        return true;
    } else if (c >= 'a' && c <= 'f') {
        value = c - 'a' + 10;
        return true;
    }
    return false;
}

} // namespace flc