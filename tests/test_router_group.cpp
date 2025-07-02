/**
 * @file test_router_group.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Unit tests for router_group class
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../include/router/router.hpp"
#include "../include/router/router_group.hpp"
#include "../include/router/router_impl.hpp"

using namespace flc;

// Simple handler for testing
class TestHandler
{
public:
    std::string name;
    mutable int call_count = 0;

    explicit TestHandler(const std::string &handler_name) : name(handler_name) {}

    void handle() const
    {
        ++call_count;
        std::cout << "Handler: " << name << " called (count: " << call_count << ")" << std::endl;
    }

    void operator()() const { handle(); }
};

// Performance timer utility
class Timer
{
private:
    std::chrono::high_resolution_clock::time_point start_time;

public:
    Timer() : start_time(std::chrono::high_resolution_clock::now()) {}

    double elapsed_ms() const
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return duration.count() / 1000.0;
    }

    void reset() { start_time = std::chrono::high_resolution_clock::now(); }
};

// Test suite class
class RouterGroupTest
{
public:
    void run_all_tests()
    {
        std::cout << "Running comprehensive router_group tests..." << std::endl;

        // Basic functionality tests
        test_basic_group_creation();
        test_group_route_registration();
        test_nested_groups();
        test_middleware_application();
        test_path_building();
        test_group_prefix_normalization();
        test_multiple_nested_groups();
        test_group_inheritance();
        test_all_http_methods();
        test_any_method();

        // Advanced functionality tests
        test_complex_routing_patterns();
        test_parameter_extraction();
        test_wildcard_routes();
        test_middleware_chains();
        test_error_handling();
        test_edge_cases();
        test_route_conflicts();
        test_url_encoding();

        // Performance tests
        test_performance_large_groups();
        test_performance_deep_nesting();
        test_performance_many_middlewares();
        test_performance_concurrent_access();
        test_cache_performance();

        // Extended Comprehensive Tests
        test_group_static_routes_comprehensive();
        test_group_parameterized_routes_comprehensive();
        test_group_wildcard_routes_comprehensive();
        test_group_middleware_comprehensive();
        test_group_large_scale_performance();

        std::cout << "\n=== All router_group tests passed! ===" << std::endl;
        print_test_summary();
    }

private:
    int total_tests = 0;
    int passed_tests = 0;

    void test_passed(const std::string &test_name)
    {
        ++total_tests;
        ++passed_tests;
        std::cout << "✓ " << test_name << " test passed" << std::endl;
    }

    void print_test_summary()
    {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << (total_tests - passed_tests) << std::endl;
        std::cout << "Success rate: " << (100.0 * passed_tests / total_tests) << "%" << std::endl;
    }

    // === Basic Tests ===

    void test_basic_group_creation()
    {
        std::cout << "Test: Basic group creation" << std::endl;

        router<TestHandler> r;
        auto group = r.group("/api");

        assert(group != nullptr);
        assert(group->get_prefix() == "/api");

        test_passed("Basic group creation");
    }

    void test_group_route_registration()
    {
        std::cout << "Test: Group route registration" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        auto handler = std::make_shared<TestHandler>("users_handler");
        api_group->get("/users", handler);

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/users", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler != nullptr);
        assert(found_handler->name == "users_handler");

        test_passed("Group route registration");
    }

    void test_nested_groups()
    {
        std::cout << "Test: Nested groups" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");
        auto v1_group = api_group->group("/v1");

        auto handler = std::make_shared<TestHandler>("v1_users_handler");
        v1_group->get("/users", handler);

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/v1/users", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler != nullptr);
        assert(found_handler->name == "v1_users_handler");

        test_passed("Nested groups");
    }

    void test_middleware_application()
    {
        std::cout << "Test: Middleware application" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original_handler = handler;
            handler = std::make_shared<TestHandler>(original_handler->name + "_with_middleware");
        });

        auto original_handler = std::make_shared<TestHandler>("original_handler");
        api_group->get("/test", original_handler);

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/test", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler != nullptr);
        assert(found_handler->name == "original_handler_with_middleware");

        test_passed("Middleware application");
    }

    void test_path_building()
    {
        std::cout << "Test: Path building" << std::endl;

        router<TestHandler> r;
        auto group = r.group("/api");

        assert(group->build_full_path("/users") == "/api/users");
        assert(group->build_full_path("users") == "/api/users");
        assert(group->build_full_path("") == "/api");
        assert(group->build_full_path("/") == "/api");

        auto root_group = r.group("");
        assert(root_group->build_full_path("/users") == "/users");
        assert(root_group->build_full_path("users") == "users");

        test_passed("Path building");
    }

    void test_group_prefix_normalization()
    {
        std::cout << "Test: Group prefix normalization" << std::endl;

        router<TestHandler> r;

        auto group1 = r.group("api");
        auto group2 = r.group("/api");
        auto group3 = r.group("/api/");

        assert(group1->get_prefix() == "/api");
        assert(group2->get_prefix() == "/api");
        assert(group3->get_prefix() == "/api");

        test_passed("Group prefix normalization");
    }

    void test_multiple_nested_groups()
    {
        std::cout << "Test: Multiple nested groups" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");
        auto v1_group = api_group->group("/v1");
        auto users_group = v1_group->group("/users");

        auto handler = std::make_shared<TestHandler>("deep_handler");
        users_group->get("/:id", handler);

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/v1/users/123", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler != nullptr);
        assert(found_handler->name == "deep_handler");
        assert(params["id"] == "123");

        test_passed("Multiple nested groups");
    }

    void test_group_inheritance()
    {
        std::cout << "Test: Group middleware inheritance" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original_handler = handler;
            handler = std::make_shared<TestHandler>(original_handler->name + "_parent_middleware");
        });

        auto v1_group = api_group->group("/v1");

        v1_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original_handler = handler;
            handler = std::make_shared<TestHandler>(original_handler->name + "_child_middleware");
        });

        auto original_handler = std::make_shared<TestHandler>("handler");
        v1_group->get("/test", original_handler);

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/v1/test", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler != nullptr);
        assert(found_handler->name == "handler_child_middleware_parent_middleware");

        test_passed("Group middleware inheritance");
    }

    void test_all_http_methods()
    {
        std::cout << "Test: All HTTP methods" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->get("/test", std::make_shared<TestHandler>("get_handler"));
        api_group->post("/test", std::make_shared<TestHandler>("post_handler"));
        api_group->put("/test", std::make_shared<TestHandler>("put_handler"));
        api_group->delete_("/test", std::make_shared<TestHandler>("delete_handler"));
        api_group->patch("/test", std::make_shared<TestHandler>("patch_handler"));
        api_group->head("/test", std::make_shared<TestHandler>("head_handler"));
        api_group->options("/test", std::make_shared<TestHandler>("options_handler"));

        std::vector<std::pair<HttpMethod, std::string>> test_cases = {
            {HttpMethod::GET, "get_handler"},        {HttpMethod::POST, "post_handler"},
            {HttpMethod::PUT, "put_handler"},        {HttpMethod::DELETE, "delete_handler"},
            {HttpMethod::PATCH, "patch_handler"},    {HttpMethod::HEAD, "head_handler"},
            {HttpMethod::OPTIONS, "options_handler"}};

        for (const auto &[method, expected_name] : test_cases) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(method, "/api/test", found_handler, params, query_params);

            assert(result == 0);
            assert(found_handler != nullptr);
            assert(found_handler->name == expected_name);
        }

        test_passed("All HTTP methods");
    }

    void test_any_method()
    {
        std::cout << "Test: Any method registration" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        auto handler = std::make_shared<TestHandler>("any_handler");
        api_group->any("/any", handler);

        std::vector<HttpMethod> methods = {HttpMethod::GET,    HttpMethod::POST,  HttpMethod::PUT,
                                           HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD,
                                           HttpMethod::OPTIONS};

        for (auto method : methods) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(method, "/api/any", found_handler, params, query_params);

            assert(result == 0);
            assert(found_handler != nullptr);
            assert(found_handler->name == "any_handler");
        }

        test_passed("Any method registration");
    }

    // === Advanced Tests ===

    void test_complex_routing_patterns()
    {
        std::cout << "Test: Complex routing patterns" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api/v1");

        // Complex parameterized routes
        api_group->get("/users/:userId/posts/:postId/comments/:commentId",
                       std::make_shared<TestHandler>("complex_handler"));

        // Routes with multiple parameters and static parts
        api_group->get("/organizations/:orgId/projects/:projectId/files/*",
                       std::make_shared<TestHandler>("wildcard_handler"));

        // Test complex parameter extraction
        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result = r.find_route(HttpMethod::GET, "/api/v1/users/123/posts/456/comments/789",
                                  found_handler, params, query_params);

        assert(result == 0);
        assert(params["userId"] == "123");
        assert(params["postId"] == "456");
        assert(params["commentId"] == "789");

        // Test wildcard route
        result = r.find_route(HttpMethod::GET,
                              "/api/v1/organizations/myorg/projects/proj1/files/src/main.cpp",
                              found_handler, params, query_params);

        assert(result == 0);
        assert(params["orgId"] == "myorg");
        assert(params["projectId"] == "proj1");
        assert(params["*"] == "src/main.cpp");

        test_passed("Complex routing patterns");
    }

    void test_parameter_extraction()
    {
        std::cout << "Test: Parameter extraction" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Routes with various parameter patterns
        api_group->get("/users/:id", std::make_shared<TestHandler>("user_handler"));
        api_group->get("/posts/:id/comments/:commentId",
                       std::make_shared<TestHandler>("comment_handler"));
        api_group->get("/files/:filename", std::make_shared<TestHandler>("file_handler"));

        // Test different parameter values
        std::vector<std::tuple<std::string, std::map<std::string, std::string>>> test_cases = {
            {"/api/users/12345", {{"id", "12345"}}},
            {"/api/users/user_abc", {{"id", "user_abc"}}},
            {"/api/posts/100/comments/200", {{"id", "100"}, {"commentId", "200"}}},
            {"/api/files/document.pdf", {{"filename", "document.pdf"}}},
            {"/api/files/img-2023.jpg", {{"filename", "img-2023.jpg"}}}};

        for (const auto &[path, expected_params] : test_cases) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);

            assert(result == 0);
            for (const auto &[key, value] : expected_params) {
                assert(params[key] == value);
            }
        }

        test_passed("Parameter extraction");
    }

    void test_wildcard_routes()
    {
        std::cout << "Test: Wildcard routes" << std::endl;

        router<TestHandler> r;
        auto static_group = r.group("/static");

        static_group->get("/*", std::make_shared<TestHandler>("static_handler"));

        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"/static/css/style.css", "css/style.css"},
            {"/static/js/app.min.js", "js/app.min.js"},
            {"/static/images/logo.png", "images/logo.png"},
            {"/static/fonts/roboto/regular.woff2", "fonts/roboto/regular.woff2"},
            {"/static/deep/nested/path/file.txt", "deep/nested/path/file.txt"}};

        for (const auto &[path, expected_wildcard] : test_cases) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);

            assert(result == 0);
            assert(params["*"] == expected_wildcard);
        }

        test_passed("Wildcard routes");
    }

    void test_middleware_chains()
    {
        std::cout << "Test: Middleware chains" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Add multiple middlewares
        api_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original = handler;
            handler = std::make_shared<TestHandler>(original->name + "_auth");
        });

        api_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original = handler;
            handler = std::make_shared<TestHandler>(original->name + "_cors");
        });

        api_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original = handler;
            handler = std::make_shared<TestHandler>(original->name + "_logging");
        });

        auto original_handler = std::make_shared<TestHandler>("handler");
        api_group->get("/test", original_handler);

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/test", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler->name == "handler_logging_cors_auth");

        test_passed("Middleware chains");
    }

    void test_error_handling()
    {
        std::cout << "Test: Error handling" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Test invalid routes
        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        // Non-existent route
        int result =
            r.find_route(HttpMethod::GET, "/api/nonexistent", found_handler, params, query_params);
        assert(result == -1);

        // Wrong method
        api_group->get("/users", std::make_shared<TestHandler>("get_only"));
        result = r.find_route(HttpMethod::POST, "/api/users", found_handler, params, query_params);
        assert(result == -1);

        test_passed("Error handling");
    }

    void test_edge_cases()
    {
        std::cout << "Test: Edge cases" << std::endl;

        router<TestHandler> r;

        // Empty prefix group
        auto empty_group = r.group("");
        empty_group->get("/test", std::make_shared<TestHandler>("empty_prefix"));

        // Root path group
        auto root_group = r.group("/");
        root_group->get("/root", std::make_shared<TestHandler>("root_prefix"));

        // Very long paths
        auto deep_group = r.group("/very/deep/nested/group/structure");
        deep_group->get("/endpoint", std::make_shared<TestHandler>("deep_handler"));

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        // Test empty prefix
        int result = r.find_route(HttpMethod::GET, "/test", found_handler, params, query_params);
        assert(result == 0);
        assert(found_handler->name == "empty_prefix");

        // Test root prefix
        result = r.find_route(HttpMethod::GET, "/root", found_handler, params, query_params);
        assert(result == 0);
        assert(found_handler->name == "root_prefix");

        // Test deep nesting
        result = r.find_route(HttpMethod::GET, "/very/deep/nested/group/structure/endpoint",
                              found_handler, params, query_params);
        assert(result == 0);
        assert(found_handler->name == "deep_handler");

        test_passed("Edge cases");
    }

    void test_route_conflicts()
    {
        std::cout << "Test: Route conflicts" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Register conflicting routes (last one should win)
        api_group->get("/users", std::make_shared<TestHandler>("first_handler"));
        api_group->get("/users", std::make_shared<TestHandler>("second_handler"));

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        int result =
            r.find_route(HttpMethod::GET, "/api/users", found_handler, params, query_params);

        assert(result == 0);
        assert(found_handler->name == "second_handler");

        test_passed("Route conflicts");
    }

    void test_url_encoding()
    {
        std::cout << "Test: URL encoding" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->get("/search", std::make_shared<TestHandler>("search_handler"));

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        // Test URL encoded query parameters
        int result = r.find_route(HttpMethod::GET, "/api/search?q=hello%20world&filter=type%3Duser",
                                  found_handler, params, query_params);

        assert(result == 0);
        assert(query_params["q"] == "hello world");
        assert(query_params["filter"] == "type=user");

        test_passed("URL encoding");
    }

    // === Performance Tests ===

    void test_performance_large_groups()
    {
        std::cout << "Test: Performance with large groups" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        const int num_routes = 1000;
        Timer timer;

        // Register many routes
        for (int i = 0; i < num_routes; ++i) {
            std::string path = "/route" + std::to_string(i);
            api_group->get(path, std::make_shared<TestHandler>("handler_" + std::to_string(i)));
        }

        double registration_time = timer.elapsed_ms();

        // Test route lookup performance
        timer.reset();
        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        for (int i = 0; i < 100; ++i) {
            int route_id = i % num_routes;
            std::string path = "/api/route" + std::to_string(route_id);
            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);
            assert(result == 0);
        }

        double lookup_time = timer.elapsed_ms();

        std::cout << "  Registration time for " << num_routes << " routes: " << registration_time
                  << "ms" << std::endl;
        std::cout << "  Average lookup time: " << lookup_time / 100.0 << "ms per lookup"
                  << std::endl;

        test_passed("Performance with large groups");
    }

    void test_performance_deep_nesting()
    {
        std::cout << "Test: Performance with deep nesting" << std::endl;

        router<TestHandler> r;

        const int depth = 10; // Reduce depth to avoid excessive memory usage
        Timer timer;

        // Store all groups to keep them alive
        std::vector<std::shared_ptr<router_group<TestHandler>>> groups;
        groups.push_back(r.group("/level0"));

        // Create deeply nested groups
        for (int i = 1; i < depth; ++i) {
            auto next_group = groups.back()->group("/level" + std::to_string(i));
            groups.push_back(next_group);
        }

        groups.back()->get("/endpoint", std::make_shared<TestHandler>("deep_handler"));
        double creation_time = timer.elapsed_ms();

        // Test lookup performance
        timer.reset();
        std::string deep_path = "";
        for (int i = 0; i < depth; ++i) {
            deep_path += "/level" + std::to_string(i);
        }
        deep_path += "/endpoint";

        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        for (int i = 0; i < 100; ++i) {
            int result =
                r.find_route(HttpMethod::GET, deep_path, found_handler, params, query_params);
            assert(result == 0);
        }

        double lookup_time = timer.elapsed_ms();

        std::cout << "  Deep nesting (" << depth << " levels) creation time: " << creation_time
                  << "ms" << std::endl;
        std::cout << "  Average deep lookup time: " << lookup_time / 100.0 << "ms per lookup"
                  << std::endl;

        test_passed("Performance with deep nesting");
    }

    void test_performance_many_middlewares()
    {
        std::cout << "Test: Performance with many middlewares" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        const int num_middlewares = 50;
        Timer timer;

        // Add many middlewares
        for (int i = 0; i < num_middlewares; ++i) {
            api_group->use([i](std::shared_ptr<TestHandler> &handler) {
                auto original = handler;
                handler = std::make_shared<TestHandler>(original->name + "_mw" + std::to_string(i));
            });
        }

        api_group->get("/test", std::make_shared<TestHandler>("base_handler"));
        double middleware_time = timer.elapsed_ms();

        // Test middleware application performance
        timer.reset();
        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        for (int i = 0; i < 100; ++i) {
            int result =
                r.find_route(HttpMethod::GET, "/api/test", found_handler, params, query_params);
            assert(result == 0);
        }

        double lookup_time = timer.elapsed_ms();

        std::cout << "  Middleware setup (" << num_middlewares
                  << " middlewares) time: " << middleware_time << "ms" << std::endl;
        std::cout << "  Average lookup with middlewares: " << lookup_time / 100.0 << "ms per lookup"
                  << std::endl;

        test_passed("Performance with many middlewares");
    }

    void test_performance_concurrent_access()
    {
        std::cout << "Test: Performance with concurrent access" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Setup routes
        for (int i = 0; i < 100; ++i) {
            api_group->get("/route" + std::to_string(i),
                           std::make_shared<TestHandler>("handler_" + std::to_string(i)));
        }

        const int num_threads = 8;
        const int requests_per_thread = 1000;

        Timer timer;
        std::vector<std::future<void>> futures;

        // Launch concurrent lookups
        for (int t = 0; t < num_threads; ++t) {
            futures.push_back(std::async(std::launch::async, [&r, requests_per_thread]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, 99);

                for (int i = 0; i < requests_per_thread; ++i) {
                    std::shared_ptr<TestHandler> found_handler;
                    std::map<std::string, std::string> params;
                    std::map<std::string, std::string> query_params;

                    int route_id = dis(gen);
                    std::string path = "/api/route" + std::to_string(route_id);
                    int result =
                        r.find_route(HttpMethod::GET, path, found_handler, params, query_params);
                    assert(result == 0);
                }
            }));
        }

        // Wait for all threads to complete
        for (auto &future : futures) {
            future.get();
        }

        double total_time = timer.elapsed_ms();
        int total_requests = num_threads * requests_per_thread;

        std::cout << "  Concurrent access (" << num_threads << " threads, " << total_requests
                  << " total requests): " << total_time << "ms" << std::endl;
        std::cout << "  Throughput: " << (total_requests / total_time * 1000.0)
                  << " requests/second" << std::endl;

        test_passed("Performance with concurrent access");
    }

    void test_cache_performance()
    {
        std::cout << "Test: Cache performance" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Setup parameterized route
        api_group->get("/users/:id/posts/:postId", std::make_shared<TestHandler>("cached_handler"));

        Timer timer;
        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        // First lookup (no cache)
        int result = r.find_route(HttpMethod::GET, "/api/users/123/posts/456", found_handler,
                                  params, query_params);
        double first_lookup = timer.elapsed_ms();
        assert(result == 0);

        // Subsequent lookups (with cache)
        timer.reset();
        for (int i = 0; i < 1000; ++i) {
            result = r.find_route(HttpMethod::GET, "/api/users/123/posts/456", found_handler,
                                  params, query_params);
            assert(result == 0);
        }
        double cached_lookups = timer.elapsed_ms();

        std::cout << "  First lookup (no cache): " << first_lookup << "ms" << std::endl;
        std::cout << "  Average cached lookup: " << cached_lookups / 1000.0 << "ms" << std::endl;
        std::cout << "  Cache speedup: " << (first_lookup / (cached_lookups / 1000.0)) << "x"
                  << std::endl;

        test_passed("Cache performance");
    }

    // === Extended Comprehensive Tests ===

    void test_group_static_routes_comprehensive()
    {
        std::cout << "Test: Group static routes comprehensive" << std::endl;

        router<TestHandler> r;

        // Create API v1 group
        auto api_v1 = r.group("/api/v1");
        auto admin_group = r.group("/admin");
        auto public_group = r.group("/public");

        // Add various static routes to different groups
        std::vector<std::tuple<std::string, std::string, std::string>> group_routes = {
            {"/users", "get_users", "api_v1"},
            {"/posts", "get_posts", "api_v1"},
            {"/categories", "get_categories", "api_v1"},
            {"/health", "health_check", "api_v1"},
            {"/status", "status_check", "api_v1"},
            {"/dashboard", "admin_dashboard", "admin"},
            {"/settings", "admin_settings", "admin"},
            {"/users", "admin_users", "admin"},
            {"/logs", "admin_logs", "admin"},
            {"/about", "public_about", "public"},
            {"/contact", "public_contact", "public"},
            {"/blog", "public_blog", "public"}};

        // Register routes to respective groups
        for (const auto &[path, handler_name, group_name] : group_routes) {
            if (group_name == "api_v1") {
                api_v1->get(path, std::make_shared<TestHandler>(handler_name));
            } else if (group_name == "admin") {
                admin_group->get(path, std::make_shared<TestHandler>(handler_name));
            } else if (group_name == "public") {
                public_group->get(path, std::make_shared<TestHandler>(handler_name));
            }
        }

        // Test all routes
        for (const auto &[path, handler_name, group_name] : group_routes) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            std::string full_path;
            if (group_name == "api_v1") {
                full_path = "/api/v1" + path;
            } else {
                full_path = "/" + group_name + path;
            }

            int result =
                r.find_route(HttpMethod::GET, full_path, found_handler, params, query_params);
            assert(result == 0);
            assert(found_handler != nullptr);
            assert(found_handler->name == handler_name);
            assert(params.empty());
        }

        test_passed("Group static routes comprehensive");
    }

    void test_group_parameterized_routes_comprehensive()
    {
        std::cout << "Test: Group parameterized routes comprehensive" << std::endl;

        router<TestHandler> r;

        auto api_group = r.group("/api");
        auto v1_group = api_group->group("/v1");
        auto v2_group = api_group->group("/v2");

        // Single parameter routes in different groups
        v1_group->get("/users/:id", std::make_shared<TestHandler>("v1_user_by_id"));
        v1_group->get("/posts/:slug", std::make_shared<TestHandler>("v1_post_by_slug"));

        v2_group->get("/users/:userId", std::make_shared<TestHandler>("v2_user_by_id"));
        v2_group->get("/articles/:articleId", std::make_shared<TestHandler>("v2_article_by_id"));

        // Multiple parameter routes
        v1_group->get("/users/:userId/posts/:postId",
                      std::make_shared<TestHandler>("v1_user_post"));
        v2_group->get("/organizations/:orgId/projects/:projectId",
                      std::make_shared<TestHandler>("v2_org_project"));
        v2_group->get("/users/:userId/profile/:profileId/settings/:settingId",
                      std::make_shared<TestHandler>("v2_complex_route"));

        // Test cases with expected parameters
        std::vector<std::tuple<std::string, std::string, std::map<std::string, std::string>>>
            test_cases = {// V1 routes
                          {"/api/v1/users/123", "v1_user_by_id", {{"id", "123"}}},
                          {"/api/v1/posts/hello-world-2023",
                           "v1_post_by_slug",
                           {{"slug", "hello-world-2023"}}},
                          {"/api/v1/users/456/posts/789",
                           "v1_user_post",
                           {{"userId", "456"}, {"postId", "789"}}},

                          // V2 routes
                          {"/api/v2/users/user-789", "v2_user_by_id", {{"userId", "user-789"}}},
                          {"/api/v2/articles/article-123",
                           "v2_article_by_id",
                           {{"articleId", "article-123"}}},
                          {"/api/v2/organizations/tech-corp/projects/web-app",
                           "v2_org_project",
                           {{"orgId", "tech-corp"}, {"projectId", "web-app"}}},
                          {"/api/v2/users/john/profile/main/settings/privacy",
                           "v2_complex_route",
                           {{"userId", "john"}, {"profileId", "main"}, {"settingId", "privacy"}}}};

        for (const auto &[path, expected_handler, expected_params] : test_cases) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);
            assert(result == 0);
            assert(found_handler != nullptr);
            assert(found_handler->name == expected_handler);
            assert(params.size() == expected_params.size());

            for (const auto &[key, value] : expected_params) {
                assert(params[key] == value);
            }
        }

        test_passed("Group parameterized routes comprehensive");
    }

    void test_group_wildcard_routes_comprehensive()
    {
        std::cout << "Test: Group wildcard routes comprehensive" << std::endl;

        router<TestHandler> r;

        auto static_group = r.group("/static");
        auto assets_group = static_group->group("/assets");
        auto cdn_group = r.group("/cdn");

        // Different levels of wildcard routes
        static_group->get("/*", std::make_shared<TestHandler>("static_files"));
        assets_group->get("/js/*", std::make_shared<TestHandler>("js_assets"));
        assets_group->get("/css/*", std::make_shared<TestHandler>("css_assets"));
        cdn_group->get("/:version/files/*", std::make_shared<TestHandler>("versioned_files"));

        // Test cases with expected wildcards and parameters
        std::vector<std::tuple<std::string, std::string, std::map<std::string, std::string>>>
            test_cases = {
                // Static group wildcards
                {"/static/images/logo.png", "static_files", {{"*", "images/logo.png"}}},
                {"/static/fonts/roboto/regular.woff2",
                 "static_files",
                 {{"*", "fonts/roboto/regular.woff2"}}},

                // Assets group specific wildcards
                {"/static/assets/js/app.min.js", "js_assets", {{"*", "app.min.js"}}},
                {"/static/assets/js/vendor/jquery.js", "js_assets", {{"*", "vendor/jquery.js"}}},
                {"/static/assets/css/main.css", "css_assets", {{"*", "main.css"}}},
                {"/static/assets/css/themes/dark.css", "css_assets", {{"*", "themes/dark.css"}}},

                // CDN with parameters and wildcards
                {"/cdn/v1.2/files/images/banner.jpg",
                 "versioned_files",
                 {{"version", "v1.2"}, {"*", "images/banner.jpg"}}},
                {"/cdn/2023.1/files/documents/manual.pdf",
                 "versioned_files",
                 {{"version", "2023.1"}, {"*", "documents/manual.pdf"}}}};

        for (const auto &[path, expected_handler, expected_params] : test_cases) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);
            assert(result == 0);
            assert(found_handler != nullptr);
            assert(found_handler->name == expected_handler);

            for (const auto &[key, value] : expected_params) {
                assert(params[key] == value);
            }
        }

        test_passed("Group wildcard routes comprehensive");
    }

    void test_group_middleware_comprehensive()
    {
        std::cout << "Test: Group middleware comprehensive" << std::endl;

        router<TestHandler> r;

        auto api_group = r.group("/api");
        auto v1_group = api_group->group("/v1");
        auto admin_group = v1_group->group("/admin");

        // Add middleware at different levels
        api_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original = handler;
            handler = std::make_shared<TestHandler>(original->name + "_api_auth");
        });

        v1_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original = handler;
            handler = std::make_shared<TestHandler>(original->name + "_v1_version");
        });

        admin_group->use([](std::shared_ptr<TestHandler> &handler) {
            auto original = handler;
            handler = std::make_shared<TestHandler>(original->name + "_admin_perm");
        });

        // Add routes at different levels
        api_group->get("/health", std::make_shared<TestHandler>("health"));
        v1_group->get("/status", std::make_shared<TestHandler>("status"));
        admin_group->get("/users", std::make_shared<TestHandler>("admin_users"));

        // Test middleware inheritance
        std::vector<std::tuple<std::string, std::string>> test_cases = {
            {"/api/health", "health_api_auth"},               // Only API middleware
            {"/api/v1/status", "status_v1_version_api_auth"}, // API + V1 middleware
            {"/api/v1/admin/users", "admin_users_admin_perm_v1_version_api_auth"} // All middlewares
        };

        for (const auto &[path, expected_handler] : test_cases) {
            std::shared_ptr<TestHandler> found_handler;
            std::map<std::string, std::string> params;
            std::map<std::string, std::string> query_params;

            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);
            assert(result == 0);
            assert(found_handler != nullptr);
            assert(found_handler->name == expected_handler);
        }

        test_passed("Group middleware comprehensive");
    }

    void test_group_large_scale_performance()
    {
        std::cout << "Test: Group large scale performance" << std::endl;

        router<TestHandler> r;

        const int num_groups = 50;
        const int routes_per_group = 20; // Reduced for stability

        Timer timer;

        // Create multiple groups with many routes each
        std::vector<std::shared_ptr<router_group<TestHandler>>> groups;

        for (int g = 0; g < num_groups; ++g) {
            auto group = r.group("/group" + std::to_string(g));

            // Add middleware to each group
            group->use([g](std::shared_ptr<TestHandler> &handler) {
                auto original = handler;
                handler = std::make_shared<TestHandler>(original->name + "_g" + std::to_string(g));
            });

            // Add routes to each group
            for (int r = 0; r < routes_per_group; ++r) {
                std::string path = "/route" + std::to_string(r);
                group->get(path, std::make_shared<TestHandler>("handler_" + std::to_string(g) +
                                                               "_" + std::to_string(r)));
            }

            groups.push_back(group);
        }

        double registration_time = timer.elapsed_ms();
        int total_routes = num_groups * routes_per_group;

        std::cout << "  Registered " << total_routes << " routes in " << num_groups << " groups in "
                  << registration_time << "ms" << std::endl;

        // Test lookup performance
        timer.reset();
        std::shared_ptr<TestHandler> found_handler;
        std::map<std::string, std::string> params;
        std::map<std::string, std::string> query_params;

        const int lookup_iterations = 500;
        int successful_lookups = 0;

        for (int i = 0; i < lookup_iterations; ++i) {
            int group_id = i % num_groups;
            int route_id = i % routes_per_group;

            std::string path =
                "/group" + std::to_string(group_id) + "/route" + std::to_string(route_id);
            int result = r.find_route(HttpMethod::GET, path, found_handler, params, query_params);

            if (result == 0) {
                successful_lookups++;
            }

            params.clear();
            query_params.clear();
        }

        double lookup_time = timer.elapsed_ms();

        std::cout << "  Performed " << lookup_iterations << " lookups in " << lookup_time << "ms"
                  << std::endl;
        std::cout << "  Successful lookups: " << successful_lookups << "/" << lookup_iterations
                  << std::endl;
        std::cout << "  Average lookup time: " << lookup_time / lookup_iterations << "ms"
                  << std::endl;
        std::cout << "  Throughput: " << (lookup_iterations / lookup_time * 1000.0)
                  << " lookups/second" << std::endl;

        assert(successful_lookups == lookup_iterations);

        test_passed("Group large scale performance");
    }
};


