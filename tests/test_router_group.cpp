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

        api_group->get("/users", TestHandler("users_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/users", params, query_params);

        assert(result.has_value());
        assert(result->get().handler != nullptr);
        assert(result->get().handler->name == "users_handler");

        test_passed("Group route registration");
    }

    void test_nested_groups()
    {
        std::cout << "Test: Nested groups" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");
        auto v1_group = api_group->group("/v1");

        api_group->get("/users", TestHandler("v1_users_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/v1/users", params, query_params);

        assert(result.has_value());
        assert(result->get().handler != nullptr);
        assert(result->get().handler->name == "v1_users_handler");

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

        api_group->get("/test", TestHandler("original_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/test", params, query_params);

        assert(result.has_value());
        assert(result->get().handler != nullptr);
        assert(result->get().handler->name == "original_handler_with_middleware");

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

        users_group->get("/:id", TestHandler("deep_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/v1/users/123", params, query_params);

        assert(result.has_value());
        assert(result->get().handler != nullptr);
        assert(result->get().handler->name == "deep_handler");
        assert(result->get().params["id"] == "123");

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

        v1_group->get("/test", TestHandler("handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/v1/test", params, query_params);

        assert(result.has_value());
        assert(result->get().handler != nullptr);
        assert(result->get().handler->name == "handler_child_middleware_parent_middleware");

        test_passed("Group middleware inheritance");
    }

    void test_all_http_methods()
    {
        std::cout << "Test: All HTTP methods" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->get("/test", TestHandler("get_handler"));
        api_group->post("/test", TestHandler("post_handler"));
        api_group->put("/test", TestHandler("put_handler"));
        api_group->delete_("/test", TestHandler("delete_handler"));
        api_group->patch("/test", TestHandler("patch_handler"));
        api_group->head("/test", TestHandler("head_handler"));
        api_group->options("/test", TestHandler("options_handler"));

        std::vector<std::pair<HttpMethod, std::string>> test_cases = {
            {HttpMethod::GET, "get_handler"},        {HttpMethod::POST, "post_handler"},
            {HttpMethod::PUT, "put_handler"},        {HttpMethod::DELETE, "delete_handler"},
            {HttpMethod::PATCH, "patch_handler"},    {HttpMethod::HEAD, "head_handler"},
            {HttpMethod::OPTIONS, "options_handler"}};

        for (const auto &[method, expected_name] : test_cases) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(method, "/api/test", params, query_params);

            assert(result.has_value());
            assert(result->get().handler != nullptr);
            assert(result->get().handler->name == expected_name);
        }

        test_passed("All HTTP methods");
    }

    void test_any_method()
    {
        std::cout << "Test: Any method registration" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->any("/any", TestHandler("any_handler"));

        std::vector<HttpMethod> methods = {HttpMethod::GET,    HttpMethod::POST,  HttpMethod::PUT,
                                           HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD,
                                           HttpMethod::OPTIONS};

        for (auto method : methods) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(method, "/api/any", params, query_params);

            assert(result.has_value());
            assert(result->get().handler != nullptr);
            assert(result->get().handler->name == "any_handler");
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
                       TestHandler("complex_handler"));

        // Routes with multiple parameters and static parts
        api_group->get("/organizations/:orgId/projects/:projectId/files/*",
                       TestHandler("wildcard_handler"));

        // Test complex parameter extraction
        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/v1/users/123/posts/456/comments/789",
                                   params, query_params);

        assert(result.has_value());
        assert(result->get().params["userId"] == "123");
        assert(result->get().params["postId"] == "456");
        assert(result->get().params["commentId"] == "789");

        // Test wildcard route
        result = r.find_route(HttpMethod::GET,
                              "/api/v1/organizations/myorg/projects/proj1/files/src/main.cpp",
                              params, query_params);

        assert(result.has_value());
        assert(result->get().params["orgId"] == "myorg");
        assert(result->get().params["projectId"] == "proj1");
        assert(result->get().params["*"] == "src/main.cpp");

        test_passed("Complex routing patterns");
    }

    void test_parameter_extraction()
    {
        std::cout << "Test: Parameter extraction" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Routes with various parameter patterns
        api_group->get("/users/:id", TestHandler("user_handler"));
        api_group->get("/posts/:id/comments/:commentId", TestHandler("comment_handler"));
        api_group->get("/files/:filename", TestHandler("file_handler"));

        // Test different parameter values
        std::vector<std::tuple<std::string, std::map<std::string, std::string>>> test_cases = {
            {"/api/users/12345", {{"id", "12345"}}},
            {"/api/users/user_abc", {{"id", "user_abc"}}},
            {"/api/posts/100/comments/200", {{"id", "100"}, {"commentId", "200"}}},
            {"/api/files/document.pdf", {{"filename", "document.pdf"}}},
            {"/api/files/img-2023.jpg", {{"filename", "img-2023.jpg"}}}};

        for (const auto &[path, expected_params] : test_cases) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);

            assert(result.has_value());
            for (const auto &[key, value] : expected_params) {
                assert(result->get().params[key] == value);
            }
        }

        test_passed("Parameter extraction");
    }

    void test_wildcard_routes()
    {
        std::cout << "Test: Wildcard routes" << std::endl;

        router<TestHandler> r;
        auto static_group = r.group("/static");

        static_group->get("/*", TestHandler("static_handler"));

        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"/static/css/style.css", "css/style.css"},
            {"/static/js/app.min.js", "js/app.min.js"},
            {"/static/images/logo.png", "images/logo.png"},
            {"/static/fonts/roboto/regular.woff2", "fonts/roboto/regular.woff2"},
            {"/static/deep/nested/path/file.txt", "deep/nested/path/file.txt"}};

        for (const auto &[path, expected_wildcard] : test_cases) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);

            assert(result.has_value());
            assert(result->get().params["*"] == expected_wildcard);
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

        api_group->get("/test", TestHandler("handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/test", params, query_params);

        assert(result.has_value());
        assert(result->get().handler->name == "handler_logging_cors_auth");

        test_passed("Middleware chains");
    }

    void test_error_handling()
    {
        std::cout << "Test: Error handling" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Test invalid routes
        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/nonexistent", params, query_params);
        assert(!result.has_value());

        // Wrong method
        api_group->get("/users", TestHandler("get_only"));
        result = r.find_route(HttpMethod::POST, "/api/users", params, query_params);
        assert(!result.has_value());

        test_passed("Error handling");
    }

    void test_edge_cases()
    {
        std::cout << "Test: Edge cases" << std::endl;

        router<TestHandler> r;

        // Empty prefix group
        auto empty_group = r.group("");
        empty_group->get("/test", TestHandler("empty_prefix"));

        // Root path group
        auto root_group = r.group("/");
        root_group->get("/root", TestHandler("root_prefix"));

        // Very long paths
        auto deep_group = r.group("/very/deep/nested/group/structure");
        deep_group->get("/endpoint", TestHandler("deep_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/test", params, query_params);

        assert(result.has_value());
        assert(result->get().handler->name == "empty_prefix");

        result = r.find_route(HttpMethod::GET, "/root", params, query_params);
        assert(result.has_value());
        assert(result->get().handler->name == "root_prefix");

        result = r.find_route(HttpMethod::GET, "/very/deep/nested/group/structure/endpoint", params,
                              query_params);
        assert(result.has_value());
        assert(result->get().handler->name == "deep_handler");

        test_passed("Edge cases");
    }

    void test_route_conflicts()
    {
        std::cout << "Test: Route conflicts" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        // Register conflicting routes (last one should win)
        api_group->get("/users", TestHandler("first_handler"));
        api_group->get("/users", TestHandler("second_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result = r.find_route(HttpMethod::GET, "/api/users", params, query_params);

        assert(result.has_value());
        assert(result->get().handler->name == "second_handler");

        test_passed("Route conflicts");
    }

    void test_url_encoding()
    {
        std::cout << "Test: URL encoding" << std::endl;

        router<TestHandler> r;
        auto api_group = r.group("/api");

        api_group->get("/search", TestHandler("search_handler"));

        std::map<std::string, std::string> params, query_params;
        auto result =
            r.find_route(HttpMethod::GET, "/api/search?q=hello%20world&filter=type%3Duser", params,
                         query_params);

        assert(result.has_value());
        assert(result->get().query_params["q"] == "hello world");
        assert(result->get().query_params["filter"] == "type=user");

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
            api_group->get(path, TestHandler("handler_" + std::to_string(i)));
        }

        double registration_time = timer.elapsed_ms();

        // Test route lookup performance
        timer.reset();
        for (int i = 0; i < 100; ++i) {
            int route_id = i % num_routes;
            std::string path = "/api/route" + std::to_string(route_id);
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);
            assert(result.has_value());
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

        groups.back()->get("/endpoint", TestHandler("deep_handler"));
        double creation_time = timer.elapsed_ms();

        // Test lookup performance
        timer.reset();
        std::string deep_path = "";
        for (int i = 0; i < depth; ++i) {
            deep_path += "/level" + std::to_string(i);
        }
        deep_path += "/endpoint";

        for (int i = 0; i < 100; ++i) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, deep_path, params, query_params);
            assert(result.has_value());
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

        api_group->get("/test", TestHandler("base_handler"));
        double middleware_time = timer.elapsed_ms();

        // Test middleware application performance
        timer.reset();
        for (int i = 0; i < 100; ++i) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, "/api/test", params, query_params);
            assert(result.has_value());
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
                           TestHandler("handler_" + std::to_string(i)));
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
                    int route_id = dis(gen);
                    std::string path = "/api/route" + std::to_string(route_id);
                    std::map<std::string, std::string> params, query_params;
                    auto result = r.find_route(HttpMethod::GET, path, params, query_params);
                    assert(result.has_value());
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
        api_group->get("/users/:id/posts/:postId", TestHandler("cached_handler"));

        Timer timer;
        std::map<std::string, std::string> params, query_params;
        auto result =
            r.find_route(HttpMethod::GET, "/api/users/123/posts/456", params, query_params);
        double first_lookup = timer.elapsed_ms();
        assert(result.has_value());

        // Subsequent lookups (with cache)
        timer.reset();
        for (int i = 0; i < 1000; ++i) {
            result =
                r.find_route(HttpMethod::GET, "/api/users/123/posts/456", params, query_params);
            assert(result.has_value());
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
                api_v1->get(path, TestHandler(handler_name));
            } else if (group_name == "admin") {
                admin_group->get(path, TestHandler(handler_name));
            } else if (group_name == "public") {
                public_group->get(path, TestHandler(handler_name));
            }
        }

        // Test all routes
        for (const auto &[path, handler_name, group_name] : group_routes) {
            std::string full_path;
            if (group_name == "api_v1") {
                full_path = "/api/v1" + path;
            } else {
                full_path = "/" + group_name + path;
            }

            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, full_path, params, query_params);
            assert(result.has_value());
            assert(result->get().handler != nullptr);
            assert(result->get().handler->name == handler_name);
            assert(result->get().params.empty());
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
        v1_group->get("/users/:id", TestHandler("v1_user_by_id"));
        v1_group->get("/posts/:slug", TestHandler("v1_post_by_slug"));

        v2_group->get("/users/:userId", TestHandler("v2_user_by_id"));
        v2_group->get("/articles/:articleId", TestHandler("v2_article_by_id"));

        // Multiple parameter routes
        v1_group->get("/users/:userId/posts/:postId", TestHandler("v1_user_post"));
        v2_group->get("/organizations/:orgId/projects/:projectId", TestHandler("v2_org_project"));
        v2_group->get("/users/:userId/profile/:profileId/settings/:settingId",
                      TestHandler("v2_complex_route"));

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
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);
            assert(result.has_value());
            assert(result->get().handler != nullptr);
            assert(result->get().handler->name == expected_handler);
            assert(result->get().params.size() == expected_params.size());

            for (const auto &[key, value] : expected_params) {
                assert(result->get().params[key] == value);
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
        static_group->get("/*", TestHandler("static_files"));
        assets_group->get("/js/*", TestHandler("js_assets"));
        assets_group->get("/css/*", TestHandler("css_assets"));
        cdn_group->get("/:version/files/*", TestHandler("versioned_files"));

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
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);
            assert(result.has_value());
            assert(result->get().handler != nullptr);
            assert(result->get().handler->name == expected_handler);

            for (const auto &[key, value] : expected_params) {
                assert(result->get().params[key] == value);
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
        api_group->get("/health", TestHandler("health"));
        v1_group->get("/status", TestHandler("status"));
        admin_group->get("/users", TestHandler("admin_users"));

        // Test middleware inheritance
        std::vector<std::tuple<std::string, std::string>> test_cases = {
            {"/api/health", "health_api_auth"},               // Only API middleware
            {"/api/v1/status", "status_v1_version_api_auth"}, // API + V1 middleware
            {"/api/v1/admin/users", "admin_users_admin_perm_v1_version_api_auth"} // All middlewares
        };

        for (const auto &[path, expected_handler] : test_cases) {
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);
            assert(result.has_value());
            assert(result->get().handler != nullptr);
            assert(result->get().handler->name == expected_handler);
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
                group->get(path,
                           TestHandler("handler_" + std::to_string(g) + "_" + std::to_string(r)));
            }

            groups.push_back(group);
        }

        double registration_time = timer.elapsed_ms();
        int total_routes = num_groups * routes_per_group;

        std::cout << "  Registered " << total_routes << " routes in " << num_groups << " groups in "
                  << registration_time << "ms" << std::endl;

        // Test lookup performance
        timer.reset();
        for (int i = 0; i < 500; ++i) {
            int group_id = i % num_groups;
            int route_id = i % routes_per_group;

            std::string path =
                "/group" + std::to_string(group_id) + "/route" + std::to_string(route_id);
            std::map<std::string, std::string> params, query_params;
            auto result = r.find_route(HttpMethod::GET, path, params, query_params);

            assert(result.has_value());
        }

        double lookup_time = timer.elapsed_ms();

        std::cout << "  Performed 500 lookups in " << lookup_time << "ms" << std::endl;
        std::cout << "  Throughput: " << (500 / lookup_time * 1000.0) << " lookups/second"
                  << std::endl;

        test_passed("Group large scale performance");
    }
};
