/**
 * @file router_impl.hpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Template method implementations for router class to avoid circular
 * dependencies 路由器类的模板方法实现，避免循环依赖
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 * This file contains the implementation of template methods that depend on
 * router_group. It is separated from router.hpp to break circular dependency
 * between router and router_group classes.
 *
 * 此文件包含依赖于router_group的模板方法实现。
 * 它从router.hpp中分离出来，以打破router和router_group类之间的循环依赖。
 *
 * Usage / 使用方法:
 * - Include this file after both router.hpp and router_group.hpp
 * - This provides the implementation for router::group() method
 *
 * - 在包含router.hpp和router_group.hpp之后包含此文件
 * - 这为router::group()方法提供了实现
 *
 * @example
 * ```cpp
 * #include "router.hpp"
 * #include "router_group.hpp"
 * #include "router_impl.hpp"  // Must be included last
 *
 * router<MyHandler> r;
 * auto group = r.group("/api");  // This method is implemented here
 * ```
 */
#pragma once

#include "router.hpp"
#include "router_group.hpp"

namespace co {

/**
 * @brief Implementation of router group creation method
 *        路由器组创建方法的实现
 *
 * @tparam Handler The type of handler function used for route callbacks
 *                 用于路由回调的处理器函数类型
 *
 * @param prefix Path prefix for the group (e.g., "/api", "/v1")
 *               组的路径前缀（例如"/api", "/v1"）
 *
 * @return std::shared_ptr<router_group<Handler>>
 *         Shared pointer to the created router group
 *         创建的路由组的共享指针
 *
 * This method creates a new router group with the specified prefix using the
 * factory pattern from router_group class. The created group will register
 * routes with this router instance as the backend.
 *
 * 此方法使用router_group类的工厂模式创建具有指定前缀的新路由组。
 * 创建的组将使用此路由器实例作为后端注册路由。
 *
 * Features / 功能特性:
 * - Safe object creation through factory pattern / 通过工厂模式安全创建对象
 * - Automatic prefix handling and normalization / 自动前缀处理和规范化
 * - No parent group (this creates a root-level group) / 无父组（创建根级组）
 * - Full middleware support / 完整的中间件支持
 *
 * @example
 * ```cpp
 * router<MyHandler> r;
 *
 * // Create root-level groups
 * auto api_group = r.group("/api");
 * auto admin_group = r.group("/admin");
 *
 * // Use the groups
 * api_group->get("/users", users_handler);
 * admin_group->get("/dashboard", dashboard_handler);
 *
 * // Routes created: /api/users, /admin/dashboard
 * ```
 *
 * @note This implementation must be in a separate file to avoid circular
 *       dependency between router.hpp and router_group.hpp
 *       此实现必须在单独的文件中，以避免router.hpp和router_group.hpp之间的循环依赖
 *
 * @see router_group::create_group() for the underlying factory method
 *      参见router_group::create_group()了解底层工厂方法
 */
template <CallableHandler Handler>
std::shared_ptr<router_group<Handler>> router<Handler>::group(std::string_view prefix)
{
    return router_group<Handler>::create_group(*this, prefix);
}

} // namespace co