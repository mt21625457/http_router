/**
 * @file router_group.hpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Gin-style router group implementation for falcon HTTP server with
 * middleware support and route organization
 *        类似Gin的路由组实现，支持中间件和路由组织功能，用于falcon HTTP服务器
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once


#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>

#include "router.hpp"

namespace flc {

// Forward declaration
// 前向声明
template <typename Handler> class router;

/**
 * @brief Router group class implementing gin-style route grouping with
 * middleware support 实现类似Gin风格的路由组类，支持中间件功能
 *
 * @tparam Handler The type of handler function used for route callbacks
 *                 用于路由回调的处理器函数类型
 *
 * Features / 功能特性:
 * - Hierarchical route organization with prefixes / 带前缀的分层路由组织
 * - Middleware chaining and inheritance / 中间件链和继承
 * - Gin-style fluent API for route registration / Gin风格的流畅API用于路由注册
 * - Safe memory management with weak pointers / 使用弱指针的安全内存管理
 * - Support for all HTTP methods and wildcards / 支持所有HTTP方法和通配符
 * - Factory pattern for safe object creation / 工厂模式确保安全的对象创建
 *
 * @example
 * ```cpp
 * // Create router and groups
 * router<MyHandler> r;
 * auto api_group = r.group("/api");
 * auto v1_group = api_group->group("/v1");
 *
 * // Add middleware
 * api_group->use(create_logger(log_func))
 *          ->use(create_auth(auth_func));
 *
 * // Register routes
 * v1_group->get("/users/:id", user_handler)
 *         ->post("/users", create_user_handler)
 *         ->delete_("/users/:id", delete_user_handler);
 *
 * // Routes will be: /api/v1/users/:id, /api/v1/users
 * ```
 */
template <typename Handler>
class router_group
    : public std::enable_shared_from_this<router_group<Handler>> {
public:
  // Type aliases for better readability and API clarity
  // 类型别名以提高可读性和API清晰度
  using HandlerPtr = std::shared_ptr<Handler>; ///< Shared pointer to handler /
                                               ///< 处理器的共享指针
  using MiddlewareFunc = std::function<void(
      HandlerPtr &)>; ///< Middleware function type / 中间件函数类型
  using RouterRef = router<Handler> &; ///< Reference to router / 路由器引用
  using GroupPtr =
      std::shared_ptr<router_group<Handler>>; ///< Shared pointer to router
                                              ///< group / 路由组的共享指针
  using WeakGroupPtr =
      std::weak_ptr<router_group<Handler>>; ///< Weak pointer to router group /
                                            ///< 路由组的弱指针

private:
  RouterRef router_;   ///< Reference to the main router / 主路由器的引用
  std::string prefix_; ///< Path prefix for this group / 此组的路径前缀
  std::vector<MiddlewareFunc>
      middlewares_;     ///< Middleware stack for this group / 此组的中间件栈
  WeakGroupPtr parent_; ///< Weak pointer to parent group (safe from cycles) /
                        ///< 指向父组的弱指针（避免循环引用）

  /**
   * @brief Private constructor - use create_group factory method instead
   *        私有构造函数 - 请使用create_group工厂方法
   *
   * @param router Reference to the main router instance / 主路由器实例的引用
   * @param prefix Path prefix for this group / 此组的路径前缀
   * @param parent Weak pointer to parent router group / 指向父路由组的弱指针
   */
  router_group(RouterRef router, std::string_view prefix = "",
               WeakGroupPtr parent = WeakGroupPtr{})
      : router_(router), prefix_(prefix), parent_(parent) {
    normalize_prefix();
  }

public:
  /**
   * @brief Factory method to create a router group safely
   *        安全创建路由组的工厂方法
   *
   * @param router Reference to the main router instance / 主路由器实例的引用
   * @param prefix Path prefix for this group (e.g., "/api", "/v1") /
   * 此组的路径前缀（例如"/api", "/v1"）
   * @param parent Weak pointer to parent router group / 指向父路由组的弱指针
   * @return GroupPtr Shared pointer to the created router group /
   * 创建的路由组的共享指针
   *
   * This factory method ensures that router_group objects are always managed by
   * shared_ptr, which is required for the weak_ptr parent relationship to work
   * correctly.
   * 此工厂方法确保router_group对象始终由shared_ptr管理，这是弱指针父子关系正常工作的必要条件。
   *
   * @example
   * ```cpp
   * auto group = router_group<Handler>::create_group(router, "/api");
   * auto nested = router_group<Handler>::create_group(router, "/v1", group);
   * ```
   */
  static GroupPtr create_group(RouterRef router, std::string_view prefix = "",
                               WeakGroupPtr parent = WeakGroupPtr{}) {
    // Use make_shared with a custom deleter to access private constructor
    // 使用make_shared和自定义删除器来访问私有构造函数
    GroupPtr group(new router_group(router, prefix, parent));
    return group;
  }

  /**
   * @brief Copy constructor (deleted to prevent accidental copies)
   *        复制构造函数（删除以防止意外复制）
   */
  router_group(const router_group &) = delete;

  /**
   * @brief Copy assignment operator (deleted to prevent accidental copies)
   *        复制赋值操作符（删除以防止意外复制）
   */
  router_group &operator=(const router_group &) = delete;

  /**
   * @brief Move constructor (deleted since we use shared_ptr)
   *        移动构造函数（删除，因为我们使用shared_ptr）
   */
  router_group(router_group &&) = delete;

  /**
   * @brief Move assignment operator (deleted since we use shared_ptr)
   *        移动赋值操作符（删除，因为我们使用shared_ptr）
   */
  router_group &operator=(router_group &&) = delete;

  /**
   * @brief Add middleware to this route group (fluent interface)
   *        向此路由组添加中间件（流畅接口）
   *
   * @param middleware Middleware function that modifies handlers /
   * 修改处理器的中间件函数
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * Middleware functions are applied in the order they are added. Each
   * middleware can wrap the handler or perform pre/post processing.
   * 中间件函数按添加顺序应用。每个中间件可以包装处理器或执行前置/后置处理。
   *
   * @example
   * ```cpp
   * group->use(create_logger(log_func))
   *      ->use(create_auth(auth_func))
   *      ->use(create_cors({"*"}));
   * ```
   */
  router_group &use(MiddlewareFunc middleware) {
    middlewares_.push_back(std::move(middleware));
    return *this;
  }

  /**
   * @brief Register a GET route (fluent interface)
   *        注册GET路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * @example
   * ```cpp
   * group->get("/users", list_users_handler)
   *      ->get("/users/:id", get_user_handler);
   * ```
   */
  router_group &get(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::GET, path, handler);
  }

  /**
   * @brief Register a POST route (fluent interface)
   *        注册POST路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * @example
   * ```cpp
   * group->post("/users", create_user_handler)
   *      ->post("/users/:id/avatar", upload_avatar_handler);
   * ```
   */
  router_group &post(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::POST, path, handler);
  }

  /**
   * @brief Register a PUT route (fluent interface)
   *        注册PUT路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * @example
   * ```cpp
   * group->put("/users/:id", update_user_handler)
   *      ->put("/users/:id/profile", update_profile_handler);
   * ```
   */
  router_group &put(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::PUT, path, handler);
  }

  /**
   * @brief Register a DELETE route (fluent interface)
   *        注册DELETE路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * Note: Method name uses underscore to avoid conflict with C++ delete keyword
   * 注意：方法名使用下划线以避免与C++ delete关键字冲突
   *
   * @example
   * ```cpp
   * group->delete_("/users/:id", delete_user_handler)
   *      ->delete_("/users/:id/avatar", delete_avatar_handler);
   * ```
   */
  router_group &delete_(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::DELETE, path, handler);
  }

  /**
   * @brief Register a PATCH route (fluent interface)
   *        注册PATCH路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * @example
   * ```cpp
   * group->patch("/users/:id", patch_user_handler)
   *      ->patch("/users/:id/settings", patch_settings_handler);
   * ```
   */
  router_group &patch(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::PATCH, path, handler);
  }

  /**
   * @brief Register a HEAD route (fluent interface)
   *        注册HEAD路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * @example
   * ```cpp
   * group->head("/users/:id", check_user_exists_handler);
   * ```
   */
  router_group &head(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::HEAD, path, handler);
  }

  /**
   * @brief Register an OPTIONS route (fluent interface)
   *        注册OPTIONS路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * @example
   * ```cpp
   * group->options("/users/*", cors_preflight_handler);
   * ```
   */
  router_group &options(std::string_view path, HandlerPtr handler) {
    return add_route(HttpMethod::OPTIONS, path, handler);
  }

  /**
   * @brief Register route for all HTTP methods (fluent interface)
   *        为所有HTTP方法注册路由（流畅接口）
   *
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Handler for this route / 此路由的处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * This registers the same handler for GET, POST, PUT, DELETE, PATCH, HEAD,
   * and OPTIONS methods.
   * 这会为GET、POST、PUT、DELETE、PATCH、HEAD和OPTIONS方法注册相同的处理器。
   *
   * @example
   * ```cpp
   * group->any("/debug", debug_handler);  // Responds to all HTTP methods
   * ```
   */
  router_group &any(std::string_view path, HandlerPtr handler) {
    static const std::vector<HttpMethod> all_methods = {
        HttpMethod::GET,    HttpMethod::POST,  HttpMethod::PUT,
        HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD,
        HttpMethod::OPTIONS};

    for (auto method : all_methods) {
      add_route(method, path, handler);
    }
    return *this;
  }

  /**
   * @brief Create a nested route group with additional prefix
   *        创建带有附加前缀的嵌套路由组
   *
   * @param relative_prefix Additional prefix for the nested group /
   * 嵌套组的附加前缀
   * @return GroupPtr A shared pointer to the new router_group instance /
   * 新router_group实例的共享指针
   *
   * The nested group inherits all middlewares from parent groups, creating a
   * middleware chain. Prefixes are combined automatically (parent prefix +
   * relative prefix).
   * 嵌套组继承所有父组的中间件，创建中间件链。前缀自动组合（父前缀 +
   * 相对前缀）。
   *
   * @example
   * ```cpp
   * auto api_group = router.group("/api");
   * auto v1_group = api_group->group("/v1");    // Prefix: /api/v1
   * auto users_group = v1_group->group("/users"); // Prefix: /api/v1/users
   *
   * users_group->get("/:id", handler); // Final route: /api/v1/users/:id
   * ```
   */
  GroupPtr group(std::string_view relative_prefix) {
    std::string full_prefix = build_full_path(relative_prefix);

    // Create nested group with this group as parent
    // 创建以此组为父组的嵌套组
    GroupPtr nested_group =
        create_group(router_, full_prefix, this->weak_from_this());

    // Inherit parent middlewares
    // 继承父组中间件
    for (const auto &middleware : get_all_middlewares()) {
      nested_group->middlewares_.push_back(middleware);
    }

    return nested_group;
  }

  /**
   * @brief Get the full prefix path for this group
   *        获取此组的完整前缀路径
   *
   * @return std::string The complete prefix path including all parent prefixes
   * / 包含所有父前缀的完整前缀路径
   *
   * @example
   * ```cpp
   * auto api_group = router.group("/api");
   * auto v1_group = api_group->group("/v1");
   * std::string prefix = v1_group->get_prefix(); // Returns "/api/v1"
   * ```
   */
  std::string get_prefix() const { return prefix_; }

  /**
   * @brief Get all middlewares including inherited ones from parent groups
   *        获取所有中间件，包括从父组继承的中间件
   *
   * @return std::vector<MiddlewareFunc> Vector of all middlewares from root to
   * this group / 从根到此组的所有中间件向量
   *
   * The middlewares are returned in the order they should be applied:
   * root group middlewares first, then child group middlewares.
   * 中间件按应用顺序返回：根组中间件优先，然后是子组中间件。
   *
   * @example
   * ```cpp
   * auto all_middlewares = group->get_all_middlewares();
   * // Contains middlewares from parent groups + this group's middlewares
   * ```
   */
  std::vector<MiddlewareFunc> get_all_middlewares() const {
    std::vector<MiddlewareFunc> all_middlewares;

    // Collect middlewares from parent groups (recursive)
    // 从父组收集中间件（递归）
    if (auto parent = parent_.lock()) {
      auto parent_middlewares = parent->get_all_middlewares();
      all_middlewares.insert(all_middlewares.end(), parent_middlewares.begin(),
                             parent_middlewares.end());
    }

    // Add this group's middlewares
    // 添加此组的中间件
    all_middlewares.insert(all_middlewares.end(), middlewares_.begin(),
                           middlewares_.end());

    return all_middlewares;
  }

  /**
   * @brief Build the complete path by combining group prefix with relative path
   *        通过组合组前缀和相对路径构建完整路径
   *
   * @param relative_path Path relative to this group / 相对于此组的路径
   * @return std::string Complete path for route registration /
   * 用于路由注册的完整路径
   *
   * This method handles proper path concatenation, ensuring no double slashes
   * and correct path formatting.
   * 此方法处理正确的路径连接，确保没有双斜杠并格式化路径正确。
   *
   * @example
   * ```cpp
   * // If group prefix is "/api/v1" and relative_path is "/users"
   * std::string full_path = group->build_full_path("/users");
   * // Result: "/api/v1/users"
   * ```
   */
  std::string build_full_path(std::string_view relative_path) const {
    if (prefix_.empty()) {
      return std::string(relative_path);
    }

    if (relative_path.empty() || relative_path == "/") {
      return prefix_;
    }

    std::string full_path = prefix_;

    // Ensure no double slashes
    // 确保没有双斜杠
    if (!relative_path.empty() && relative_path[0] != '/' && !prefix_.empty() &&
        prefix_.back() != '/') {
      full_path += '/';
    }

    full_path += relative_path;
    return full_path;
  }

  /**
   * @brief Get parent group if it still exists
   *        如果父组仍然存在则获取父组
   *
   * @return GroupPtr Shared pointer to parent group, or nullptr if parent no
   * longer exists / 指向父组的共享指针，如果父组不再存在则为nullptr
   *
   * This method safely checks if the parent group is still alive through the
   * weak_ptr mechanism. 此方法通过weak_ptr机制安全检查父组是否仍然存在。
   *
   * @example
   * ```cpp
   * if (auto parent = group->get_parent()) {
   *     // Parent is still alive, can safely use it
   *     std::cout << "Parent prefix: " << parent->get_prefix() << std::endl;
   * }
   * ```
   */
  GroupPtr get_parent() const { return parent_.lock(); }

  /**
   * @brief Check if this group has a valid parent
   *        检查此组是否有有效的父组
   *
   * @return bool True if parent exists, false otherwise /
   * 如果父组存在返回true，否则返回false
   *
   * @example
   * ```cpp
   * if (group->has_parent()) {
   *     // This is a nested group
   * } else {
   *     // This is a root-level group
   * }
   * ```
   */
  bool has_parent() const { return !parent_.expired(); }

private:
  /**
   * @brief Normalize the prefix path (remove trailing slashes, ensure leading
   * slash) 规范化前缀路径（删除尾部斜杠，确保前导斜杠）
   *
   * This method ensures consistent path formatting across all route groups.
   * It removes trailing slashes (except for root) and ensures a leading slash.
   * 此方法确保所有路由组的路径格式一致。它删除尾部斜杠（除了根路径）并确保前导斜杠。
   */
  void normalize_prefix() {
    if (prefix_.empty()) {
      return;
    }

    // Ensure leading slash
    // 确保前导斜杠
    if (prefix_[0] != '/') {
      prefix_ = '/' + prefix_;
    }

    // Remove trailing slash (except for root)
    // 删除尾部斜杠（除了根路径）
    if (prefix_.length() > 1 && prefix_.back() == '/') {
      prefix_.pop_back();
    }
  }

  /**
   * @brief Add a route with middleware application
   *        添加带有中间件应用的路由
   *
   * @param method HTTP method / HTTP方法
   * @param path Relative path within this group / 此组内的相对路径
   * @param handler Original handler / 原始处理器
   * @return router_group& Reference to this group for method chaining /
   * 此组的引用用于方法链式调用
   *
   * This method applies all middlewares (inherited + own) to the handler before
   * registering the route with the main router.
   * 此方法在向主路由器注册路由之前，将所有中间件（继承的+自己的）应用到处理器。
   */
  router_group &add_route(HttpMethod method, std::string_view path,
                          HandlerPtr handler) {
    std::string full_path = build_full_path(path);

    // Apply all middlewares to the handler (from parent to child)
    // 将所有中间件应用到处理器（从父到子）
    HandlerPtr wrapped_handler = handler;
    auto all_middlewares = get_all_middlewares();

    // Apply middlewares in order (first middleware wraps the original handler,
    // second middleware wraps the result, etc.)
    // 按顺序应用中间件（第一个中间件包装原始处理器，第二个中间件包装结果，等等）
    for (auto it = all_middlewares.rbegin(); it != all_middlewares.rend();
         ++it) {
      (*it)(wrapped_handler);
    }

    // Register the wrapped handler with the main router
    // 向主路由器注册包装后的处理器
    router_.add_route(method, full_path, wrapped_handler);

    return *this;
  }
};

/**
 * @brief Factory function to create a router group (convenience function)
 *        创建路由组的工厂函数（便利函数）
 *
 * @tparam Handler Handler type / 处理器类型
 * @param router Reference to the main router / 主路由器的引用
 * @param prefix Path prefix for the group / 组的路径前缀
 * @return std::shared_ptr<router_group<Handler>> Shared pointer to the created
 * router group / 创建的路由组的共享指针
 *
 * This is a convenience function that wraps the router_group::create_group
 * static method. 这是一个便利函数，包装了router_group::create_group静态方法。
 *
 * @example
 * ```cpp
 * auto group = create_router_group(router, "/api");
 * ```
 */
template <typename Handler>
std::shared_ptr<router_group<Handler>>
create_router_group(router<Handler> &router, std::string_view prefix = "") {
  return router_group<Handler>::create_group(router, prefix);
}

/**
 * @brief Middleware factory functions for common use cases
 *        常见用例的中间件工厂函数
 *
 * This namespace provides ready-to-use middleware factories for common
 * scenarios like logging, authentication, and CORS handling.
 * 此命名空间为日志记录、身份验证和CORS处理等常见场景提供即用的中间件工厂。
 */
namespace middleware {

/**
 * @brief Create a logging middleware
 *        创建日志记录中间件
 *
 * @tparam Handler Handler type / 处理器类型
 * @param logger_func Function to log requests / 记录请求的函数
 * @return Middleware function / 中间件函数
 *
 * This middleware logs request processing. In a real implementation,
 * you would have access to request/response objects for more detailed logging.
 * 此中间件记录请求处理。在实际实现中，您可以访问请求/响应对象以进行更详细的日志记录。
 *
 * @example
 * ```cpp
 * auto log_middleware = create_logger([](const std::string& msg) {
 *     std::cout << "[LOG] " << msg << std::endl;
 * });
 * group->use(log_middleware);
 * ```
 */
template <typename Handler>
typename router_group<Handler>::MiddlewareFunc
create_logger(std::function<void(const std::string &)> logger_func) {
  return [logger_func](std::shared_ptr<Handler> &handler) {
    // This is a simplified example - in practice, you'd wrap the handler
    // to perform logging before/after execution
    // 这是一个简化的示例 -
    // 在实际应用中，您会包装处理器以在执行前/后进行日志记录
    auto original_handler = handler;
    handler = std::make_shared<Handler>(
        [original_handler, logger_func](auto... args) {
          logger_func("Request processed");
          return (*original_handler)(args...);
        });
  };
}

/**
 * @brief Create an authentication middleware
 *        创建身份验证中间件
 *
 * @tparam Handler Handler type / 处理器类型
 * @param auth_func Function to validate authentication / 验证身份验证的函数
 * @return Middleware function / 中间件函数
 *
 * This middleware performs authentication checks before allowing request
 * processing. 此中间件在允许请求处理之前执行身份验证检查。
 *
 * @example
 * ```cpp
 * auto auth_middleware = create_auth([]() -> bool {
 *     // Check authentication token, session, etc.
 *     return is_authenticated();
 * });
 * group->use(auth_middleware);
 * ```
 */
template <typename Handler>
typename router_group<Handler>::MiddlewareFunc
create_auth(std::function<bool()> auth_func) {
  return [auth_func](std::shared_ptr<Handler> &handler) {
    auto original_handler = handler;
    handler =
        std::make_shared<Handler>([original_handler, auth_func](auto... args) {
          if (!auth_func()) {
            // Return authorization error
            // 返回授权错误
            return; // This would be handled differently in practice
          }
          return (*original_handler)(args...);
        });
  };
}

/**
 * @brief Create a CORS middleware
 *        创建CORS中间件
 *
 * @tparam Handler Handler type / 处理器类型
 * @param allowed_origins Vector of allowed origins / 允许的来源向量
 * @return Middleware function / 中间件函数
 *
 * This middleware handles Cross-Origin Resource Sharing (CORS) by setting
 * appropriate headers for cross-origin requests.
 * 此中间件通过为跨域请求设置适当的头部来处理跨域资源共享（CORS）。
 *
 * @example
 * ```cpp
 * auto cors_middleware = create_cors({"https://example.com",
 * "https://app.com"}); group->use(cors_middleware);
 * ```
 */
template <typename Handler>
typename router_group<Handler>::MiddlewareFunc
create_cors(const std::vector<std::string> &allowed_origins = {"*"}) {
  return [allowed_origins](std::shared_ptr<Handler> &handler) {
    auto original_handler = handler;
    handler = std::make_shared<Handler>(
        [original_handler, allowed_origins](auto... args) {
          // Add CORS headers (simplified example)
          // In practice, you'd have access to response headers here
          // 添加CORS头部（简化示例）
          // 在实际应用中，您可以在这里访问响应头部
          return (*original_handler)(args...);
        });
  };
}

} // namespace middleware

} // namespace flc