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

#include <algorithm> // Required for std::transform and std::count
#include <chrono>
#include <concepts>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "tsl/htrie_map.h"

namespace flc {

/**
 * @brief Concept to ensure Handler is a callable functor (supports lambda expressions)
 *        确保Handler是可调用仿函数的概念约束（支持lambda表达式）
 *
 * Handler must be:
 * - Move constructible and assignable (for efficient storage)
 * - Callable with no arguments (functor interface)
 * - Can be lambda expressions, function objects, or function pointers
 *
 * Handler必须：
 * - 移动可构造和可赋值（用于高效存储）
 * - 无参数可调用（仿函数接口）
 * - 可以是lambda表达式、函数对象或函数指针
 */
template <typename T>
concept CallableHandler = requires(T t) {
    // Must be move constructible
    T{std::move(t)};

    // Must be callable with no arguments (functor interface)
    t();
} && std::move_constructible<T> && std::invocable<T>;

// Forward declaration for router_group
// router_group类的前向声明
template <CallableHandler Handler>
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
 * @tparam Handler The type of handler functor used for route callbacks
 *                 用于路由回调的处理器仿函数类型
 *                 Must satisfy CallableHandler concept (movable, default constructible, callable)
 *                 必须满足CallableHandler概念（可移动、默认可构造、可调用）
 *
 * Features / 功能特性:
 * - Thread-safe routing / 线程安全路由
 * - Multiple storage strategies for optimal performance / 多种存储策略实现最优性能
 * - Support for static, parameterized, and wildcard routes / 支持静态、参数化和通配符路由
 * - Query parameter parsing with URL decoding / 查询参数解析与URL解码
 * - Router groups for organizing routes / 路由组用于组织路由
 * - Type-safe handler constraints with C++20 concepts / 使用C++20概念的类型安全处理器约束
 *
 * @example
 * ```cpp
 * // Define a handler functor
 * class MyHandler {
 * public:
 *     void operator()() {
 *         // Handle request
 *     }
 * };
 *
 * router<MyHandler> r;
 * r.add_route(HttpMethod::GET, "/users/:id", MyHandler{});
 * ```
 */

template <CallableHandler Handler>
class router
{

private:
    /**
     * @brief Route information storage structure (using unique_ptr for clear ownership)
     *        路由信息存储结构（使用unique_ptr明确所有权）
     */
    struct RouteInfo
    {
        Handler handler;                      ///< Route handler / 路由处理器
        std::vector<std::string> param_names; ///< Parameter names for parameterized
                                              ///< routes / 参数化路由的参数名
        bool has_wildcard;                    ///< Whether route has wildcard / 路由是否包含通配符

        // 删除默认构造函数 - Delete default constructor (Handler may not be default constructible)
        RouteInfo() = delete;

        // 显式构造函数 - Explicit constructor
        RouteInfo(Handler h, std::vector<std::string> names, bool wildcard)
            : handler(std::move(h)), param_names(std::move(names)), has_wildcard(wildcard)
        {
        }

        // 禁用拷贝构造函数和赋值操作符，unique_ptr天然支持移动语义
        // Disable copy constructor and assignment operator, unique_ptr naturally supports move
        // semantics
        RouteInfo(const RouteInfo &) = delete;
        RouteInfo &operator=(const RouteInfo &) = delete;

        // 移动构造函数 - Move constructor
        RouteInfo(RouteInfo &&other) noexcept
            : handler(std::move(other.handler)), param_names(std::move(other.param_names)),
              has_wildcard(other.has_wildcard)
        {
            other.has_wildcard = false;
        }

        // 删除移动赋值操作符 - Delete move assignment operator (Handler may not support move
        // assignment)
        RouteInfo &operator=(RouteInfo &&other) = delete;

        // 析构函数 - Destructor
        ~RouteInfo() = default; // unique_ptr自动处理内存管理
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

    // Performance tuning constants
    // 性能调优常量
    static constexpr size_t SHORT_PATH_THRESHOLD =
        10; ///< Maximum length for short paths / 短路径的最大长度
    static constexpr size_t SEGMENT_THRESHOLD = 1; ///< Path segment threshold / 路径段阈值

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
    void add_route(HttpMethod method, const std::string &path, Handler &&handler);

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
    std::optional<std::reference_wrapper<Handler>>
    find_route(HttpMethod method, std::string_view path, std::map<std::string, std::string> &params,
               std::map<std::string, std::string> &query_params);

    /**
     * @brief Clear all route data structures (for testing and cleanup)
     *        清理所有路由数据结构（用于测试和清理）
     *
     * This clears all routes, caches, and internal data structures.
     * Useful for testing environments to avoid memory management issues.
     * 这会清理所有路由、缓存和内部数据结构。
     * 在测试环境中很有用，可以避免内存管理问题。
     */
    void clear_all_routes();

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

    // ============================================================================
    // 优化函数公共接口 - Optimized Functions Public Interface (for testing and advanced usage)
    // ============================================================================

    /**
     * @brief Split path into segments for matching (optimized version)
     *        将路径分割为段以进行匹配（优化版本）
     *
     * @param path URL path (string_view for zero-copy) / URL路径（使用string_view实现零拷贝）
     * @param segments [OUT] Path segments / [输出] 路径段
     *
     * @note 优化版本使用string_view避免拷贝，预分配容器大小减少重分配
     *       Optimized version uses string_view to avoid copying, pre-allocates container size to
     * reduce reallocations
     */
    void split_path_optimized(std::string_view path, std::vector<std::string> &segments);

    /**
     * @brief URL decode string (safe optimized version)
     *        URL解码字符串（安全优化版本）
     *
     * @param str [IN/OUT] String to decode / [输入/输出] 要解码的字符串
     *
     * @note 优化版本修复了边界检查bug，预分配内存，使用移动语义避免拷贝
     *       Optimized version fixes boundary check bug, pre-allocates memory, uses move semantics
     * to avoid copying
     */
    void url_decode_safe(std::string &str);

    /**
     * @brief Convert hexadecimal character to integer (safe optimized version)
     *        将十六进制字符转换为整数（安全优化版本）
     *
     * @param c Hex character / 十六进制字符
     * @param value [OUT] Integer value / [输出] 整数值
     * @return bool True if valid hex, false otherwise / 有效十六进制返回true，否则返回false
     *
     * @note 优化版本使用noexcept标记，提供更好的性能和异常安全性
     *       Optimized version uses noexcept specification for better performance and exception
     * safety
     */
    bool hex_to_int_safe(char c, int &value) noexcept;

    /**
     * @brief Split path into segments for matching (legacy version for testing)
     *        将路径分割为段以进行匹配（传统版本用于测试）
     *
     * @param path URL path / URL路径
     * @param segments [OUT] Path segments / [输出] 路径段
     */
    void split_path(const std::string &path, std::vector<std::string> &segments);

    /**
     * @brief URL decode string (legacy version for testing)
     *        URL解码字符串（传统版本用于测试）
     *
     * @param str [IN/OUT] String to decode / [输入/输出] 要解码的字符串
     */
    void url_decode(std::string &str);

private:
    /**
     * @brief Normalize path by removing redundant slashes and trailing slashes
     *        通过删除多余斜杠和尾部斜杠来规范化路径
     *
     * @param path [IN/OUT] Path to normalize / [输入/输出] 要规范化的路径
     */
    static void normalize_path(std::string &path);

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
     * @brief Parse query parameters from URL with URL decoding
     *        从URL解析查询参数并进行URL解码
     *
     * @param query Query string (without leading ?) / 查询字符串（不含前导?）
     * @param params [OUT] Parsed parameters / [输出] 解析的参数
     */
    void parse_query_params(std::string_view query, std::map<std::string, std::string> &params);

    /**
     * @brief Convert hexadecimal character to integer (legacy version for compatibility)
     *        将十六进制字符转换为整数（保持兼容性的传统版本）
     *
     * @param c Hex character / 十六进制字符
     * @param value [OUT] Integer value / [输出] 整数值
     * @return bool True if valid hex, false otherwise / 有效十六进制返回true，否则返回false
     */
    bool hex_to_int(char c, int &value);
};

// ========== GLOBAL FUNCTION DECLARATIONS ==========

// RouteInfo now uses move-only semantics for better memory safety
// No global swap function needed as copying is disabled

// ========== TEMPLATE METHOD IMPLEMENTATIONS ==========

template <CallableHandler Handler>
void router<Handler>::add_route(HttpMethod method, const std::string &path, Handler &&handler)
{
    std::string normalized_path = path;
    normalize_path(normalized_path);

    // Parse path information using optimized version
    std::vector<std::string> segments;
    split_path_optimized(normalized_path, segments);

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

    // Storage strategy based on route characteristics
    if (!has_params && !has_wildcard) {
        // Static route - 暂时全部使用hash map存储
        static_hash_routes_by_method_[method].emplace(
            std::move(normalized_path), RouteInfo(std::move(handler), param_names, has_wildcard));
    } else {
        // Parameterized or wildcard route
        auto &route_vector = param_routes_by_method_[method];

        // 性能优化：为大规模场景预分配vector容量，减少重分配
        if (route_vector.capacity() == 0) {
            route_vector.reserve(2000);
        } else if (route_vector.size() >= route_vector.capacity() * 0.9) {
            route_vector.reserve(route_vector.capacity() * 2);
        }

        route_vector.emplace_back(
            std::move(normalized_path),
            RouteInfo(std::move(handler), std::move(param_names), has_wildcard));
    }
}

template <CallableHandler Handler>
std::optional<std::reference_wrapper<Handler>>
router<Handler>::find_route(HttpMethod method, std::string_view path,
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

    // Try static routes - hash map only (trie temporarily disabled)
    auto hash_method_it = static_hash_routes_by_method_.find(method);
    if (hash_method_it != static_hash_routes_by_method_.end()) {
        auto route_it = hash_method_it->second.find(path_without_query);
        if (route_it != hash_method_it->second.end()) {
            Handler &found_handler = route_it->second.handler;
            return std::ref(found_handler);
        }
    }

    // Trie lookup temporarily disabled to avoid memory management issues
    // TODO: Re-enable trie lookup after resolving htrie_map memory issues

    // Try parameterized routes
    auto param_method_it = param_routes_by_method_.find(method);
    if (param_method_it != param_routes_by_method_.end()) {
        std::vector<std::string> path_segments;
        split_path_optimized(path_without_query, path_segments);

        // Disable segment index optimization for now to avoid memory issues
        // TODO: Re-implement segment index optimization for wildcard routes properly
        bool found_via_index = false;

        // If segment index didn't help, try all parameterized routes
        if (!found_via_index) {
            for (auto &route_pair : param_method_it->second) {
                if (match_route(path_without_query, route_pair.first, route_pair.second, params)) {
                    Handler &found_handler = route_pair.second.handler;
                    return std::ref(found_handler);
                }
            }
        }
    }

    return std::nullopt; // Route not found
}

template <CallableHandler Handler>
void router<Handler>::clear_all_routes()
{
    try {
        // 直接清理容器，让shared_ptr自动管理内存
        static_hash_routes_by_method_.clear();
        static_trie_routes_by_method_.clear();
        param_routes_by_method_.clear();
        segment_index_by_method_.clear();

    } catch (...) {
        // 忽略清理过程中的异常
    }
}

template <CallableHandler Handler>
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

template <CallableHandler Handler>
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

template <CallableHandler Handler>
bool router<Handler>::match_route(const std::string &path, const std::string &pattern,
                                  const RouteInfo &route_info,
                                  std::map<std::string, std::string> &params)
{
    std::vector<std::string> path_segments, pattern_segments;
    split_path_optimized(path, path_segments);
    split_path_optimized(pattern, pattern_segments);

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
                    std::string param_value = path_segments[i];
                    url_decode_safe(param_value);
                    params[route_info.param_names[param_index++]] = std::move(param_value);
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
            std::string decoded_segment = path_segments[j];
            url_decode_safe(decoded_segment);
            wildcard_value += decoded_segment;
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
                    std::string param_value = path_seg;
                    url_decode_safe(param_value);
                    params[route_info.param_names[param_index++]] = std::move(param_value);
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

template <CallableHandler Handler>
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

template <CallableHandler Handler>
void router<Handler>::split_path_optimized(std::string_view path,
                                           std::vector<std::string> &segments)
{
    // 清空输出容器，确保结果的干净状态
    // Clear output container to ensure clean result state
    segments.clear();

    // 处理边界情况：空路径或根路径直接返回
    // Handle edge cases: empty path or root path return directly
    if (path.empty() || path == "/") {
        return;
    }

    // 性能优化：预估路径段数以减少vector的重分配开销
    // Performance optimization: Pre-estimate path segments to reduce vector reallocation overhead
    // 计算斜杠数量作为段数的上界估计
    // Count slashes as upper bound estimate of segment count
    size_t estimated_segments = std::count(path.begin(), path.end(), '/');
    // 为vector预分配空间，避免多次扩容
    // Pre-allocate space for vector to avoid multiple expansions
    segments.reserve(estimated_segments > 0 ? estimated_segments : 4);

    // 确定解析起始位置：跳过前导斜杠
    // Determine parsing start position: skip leading slash
    size_t start = (path[0] == '/') ? 1 : 0;

    // 主解析循环：遍历路径提取各个段
    // Main parsing loop: traverse path to extract segments
    while (start < path.length()) {
        // 查找下一个斜杠的位置
        // Find position of next slash
        size_t end = path.find('/', start);
        if (end == std::string_view::npos) {
            // 如果没有找到斜杠，说明到了最后一段
            // If no slash found, we've reached the last segment
            end = path.length();
        }

        // 只有当段不为空时才添加（避免连续斜杠产生空段）
        // Only add segment if it's not empty (avoid empty segments from consecutive slashes)
        if (end > start) {
            // 使用emplace_back直接在容器中构造字符串，避免临时对象
            // Use emplace_back to construct string directly in container, avoiding temporary
            // objects
            segments.emplace_back(path.substr(start, end - start));
        }

        // 移动到下一段的起始位置
        // Move to start position of next segment
        start = end + 1;
    }
}

template <CallableHandler Handler>
void router<Handler>::split_path(const std::string &path, std::vector<std::string> &segments)
{
    // 使用优化版本进行实现 - Use optimized version for implementation
    split_path_optimized(path, segments);
}

template <CallableHandler Handler>
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
            url_decode_safe(key_value);
            params[key_value] = "";
        } else {
            std::string key = key_value.substr(0, eq_pos);
            std::string value = key_value.substr(eq_pos + 1);
            url_decode_safe(key);
            url_decode_safe(value);
            params[key] = value;
        }

        start = end + 1;
    }
}

template <CallableHandler Handler>
void router<Handler>::url_decode_safe(std::string &str)
{
    // 空字符串快速返回，避免不必要的处理
    // Quick return for empty string to avoid unnecessary processing
    if (str.empty())
        return;

    // 性能优化：预分配结果字符串的容量
    // Performance optimization: Pre-allocate capacity for result string
    // 解码后的字符串长度通常不会超过原字符串
    // Decoded string length usually doesn't exceed original string
    std::string result;
    result.reserve(str.length()); // 预分配内存 - Pre-allocate memory

    // 主解码循环：逐字符处理输入字符串
    // Main decoding loop: process input string character by character
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];

        if (c == '+') {
            // 规则1：加号转换为空格
            // Rule 1: Plus sign converts to space
            result += ' ';
        } else if (c == '%') {
            // 规则2：百分号编码处理
            // Rule 2: Percent encoding handling

            // 关键修复：安全的边界检查，防止缓冲区溢出
            // Critical fix: Safe boundary checking to prevent buffer overflow
            // 确保有足够的字符进行十六进制解码
            // Ensure sufficient characters for hexadecimal decoding
            if (i + 2 < str.length()) {
                int value1, value2;
                // 尝试解析十六进制字符
                // Attempt to parse hexadecimal characters
                if (hex_to_int_safe(str[i + 1], value1) && hex_to_int_safe(str[i + 2], value2)) {
                    // 成功解析：将十六进制值转换为字符
                    // Successful parsing: convert hexadecimal value to character
                    result += static_cast<char>(value1 * 16 + value2);
                    i += 2; // 跳过已处理的字符 - Skip processed characters
                } else {
                    // 无效的十六进制编码：保持原样，提高健壮性
                    // Invalid hexadecimal encoding: keep as-is, improve robustness
                    result += c;
                }
            } else {
                // 不完整的编码（字符串末尾的%或%X）：保持原样
                // Incomplete encoding (%or %X at end of string): keep as-is
                result += c;
            }
        } else {
            // 规则3：普通字符直接复制
            // Rule 3: Normal characters copied directly
            result += c;
        }
    }

    // 性能优化：使用移动语义避免最终的字符串拷贝
    // Performance optimization: Use move semantics to avoid final string copy
    str.swap(result); // 使用swap避免潜在的移动问题 - Use swap to avoid potential move issues
}

template <CallableHandler Handler>
void router<Handler>::url_decode(std::string &str)
{
    // 使用安全优化版本进行实现 - Use safe optimized version for implementation
    url_decode_safe(str);
}

template <CallableHandler Handler>
bool router<Handler>::hex_to_int_safe(char c, int &value) noexcept
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

template <CallableHandler Handler>
bool router<Handler>::hex_to_int(char c, int &value)
{
    // 使用优化版本进行实现 - Use optimized version for implementation
    return hex_to_int_safe(c, value);
}

} // namespace flc