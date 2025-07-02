/**
 * @file router_simple_test.cpp
 * @brief 简单的路由器测试，验证unique_ptr接口设计
 * @version 1.0
 * @date 2025-03-26
 */

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>

#include "router/router.hpp"

using namespace flc;

/**
 * @brief 测试用的简单处理器类
 */
class SimpleHandler
{
private:
    int id_;

public:
    explicit SimpleHandler(int id) : id_(id) {}

    // 禁用拷贝，只允许移动
    SimpleHandler(const SimpleHandler &) = delete;
    SimpleHandler &operator=(const SimpleHandler &) = delete;

    SimpleHandler(SimpleHandler &&) = default;
    SimpleHandler &operator=(SimpleHandler &&) = default;

    ~SimpleHandler() = default;

    int id() const { return id_; }
    void handle() const {}
};

/**
 * @class SimpleRouterTest
 * @brief 简单的路由器测试套件
 */
class SimpleRouterTest : public ::testing::Test
{
protected:
    router<SimpleHandler> router_;
    std::map<std::string, std::string> params_;
    std::map<std::string, std::string> query_params_;

    void SetUp() override
    {
        params_.clear();
        query_params_.clear();
        router_.clear_cache();
    }

    void TearDown() override
    {
        params_.clear();
        query_params_.clear();
        router_.clear_cache();
    }

    std::unique_ptr<SimpleHandler> make_handler(int id)
    {
        return std::make_unique<SimpleHandler>(id);
    }
};

/**
 * @brief 测试基本的路由添加和查找功能
 */
TEST_F(SimpleRouterTest, BasicAddAndFind)
{
    // 添加静态路由
    router_.add_route(HttpMethod::GET, "/api/health", make_handler(1));

    // 查找路由
    auto result = router_.find_route(HttpMethod::GET, "/api/health", params_, query_params_);

    EXPECT_NE(result, nullptr);
    if (result) {
        EXPECT_EQ(result->id(), 1);
    }
}

/**
 * @brief 测试参数化路由
 */
TEST_F(SimpleRouterTest, ParameterizedRoute)
{
    // 添加参数化路由
    router_.add_route(HttpMethod::GET, "/users/:id", make_handler(2));

    // 查找路由
    auto result = router_.find_route(HttpMethod::GET, "/users/123", params_, query_params_);

    EXPECT_NE(result, nullptr);
    if (result) {
        EXPECT_EQ(result->id(), 2);
        EXPECT_EQ(params_["id"], "123");
    }
}

/**
 * @brief 测试通配符路由
 */
TEST_F(SimpleRouterTest, WildcardRoute)
{
    // 添加通配符路由
    router_.add_route(HttpMethod::GET, "/static/*", make_handler(3));

    // 查找路由
    auto result =
        router_.find_route(HttpMethod::GET, "/static/css/main.css", params_, query_params_);

    EXPECT_NE(result, nullptr);
    if (result) {
        EXPECT_EQ(result->id(), 3);
        EXPECT_EQ(params_["*"], "css/main.css");
    }
}

/**
 * @brief 测试查询参数解析
 */
TEST_F(SimpleRouterTest, QueryParameters)
{
    // 添加静态路由
    router_.add_route(HttpMethod::GET, "/search", make_handler(4));

    // 查找带查询参数的路由
    auto result =
        router_.find_route(HttpMethod::GET, "/search?q=test&sort=date", params_, query_params_);

    EXPECT_NE(result, nullptr);
    if (result) {
        EXPECT_EQ(result->id(), 4);
        EXPECT_EQ(query_params_["q"], "test");
        EXPECT_EQ(query_params_["sort"], "date");
    }
}

/**
 * @brief 测试路由未找到的情况
 */
TEST_F(SimpleRouterTest, RouteNotFound)
{
    // 添加一个路由
    router_.add_route(HttpMethod::GET, "/api/users", make_handler(5));

    // 查找不存在的路由
    auto result = router_.find_route(HttpMethod::GET, "/api/posts", params_, query_params_);

    EXPECT_EQ(result, nullptr);
}

/**
 * @brief 测试不同HTTP方法
 */
TEST_F(SimpleRouterTest, DifferentHttpMethods)
{
    // 添加不同方法的路由
    router_.add_route(HttpMethod::GET, "/api/users", make_handler(6));
    router_.add_route(HttpMethod::POST, "/api/users", make_handler(7));

    // 测试GET方法
    auto get_result = router_.find_route(HttpMethod::GET, "/api/users", params_, query_params_);
    EXPECT_NE(get_result, nullptr);
    if (get_result) {
        EXPECT_EQ(get_result->id(), 6);
    }

    // 测试POST方法
    auto post_result = router_.find_route(HttpMethod::POST, "/api/users", params_, query_params_);
    EXPECT_NE(post_result, nullptr);
    if (post_result) {
        EXPECT_EQ(post_result->id(), 7);
    }

    // 测试不存在的方法
    auto put_result = router_.find_route(HttpMethod::PUT, "/api/users", params_, query_params_);
    EXPECT_EQ(put_result, nullptr);
}