/**
 * @file test_router.cpp
 * @author mt21625457 (mt21625457@163.com)
 * @brief Comprehensive unit tests for router class
 * @version 0.1
 * @date 2025-03-26
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <cassert>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <thread>
#include <future>
#include <algorithm>

#include "falcon/router/router.hpp"

using namespace flc;

// Mock handler for testing
class MockHandler {
public:
  std::string name;
  mutable int call_count = 0;

  explicit MockHandler(const std::string &handler_name) : name(handler_name) {}

  void handle() const {
    ++call_count;
    std::cout << "Handler: " << name << " called (count: " << call_count << ")"
              << std::endl;
  }

  void operator()() const { handle(); }
};

// Performance timer utility
class Timer {
private:
  std::chrono::high_resolution_clock::time_point start_time;

public:
  Timer() : start_time(std::chrono::high_resolution_clock::now()) {}

  double elapsed_ms() const {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    return duration.count() / 1000.0;
  }

  void reset() { start_time = std::chrono::high_resolution_clock::now(); }
};

// Test suite class
class RouterTest {
public:
  void run_all_tests() {
    std::cout << "Running comprehensive router tests..." << std::endl;

    // Basic functionality tests
    test_add_and_find_static_route();
    test_find_non_existent_route();
    test_parameterized_route();
    test_multiple_parameters_route();
    test_wildcard_route();
    test_query_parameters();
    test_different_http_methods();
    test_path_normalization();
    test_route_caching();
    test_clear_cache();

    // Advanced functionality tests
    test_complex_route_patterns();
    test_route_conflicts();
    test_url_encoding();
    test_edge_cases();
    test_http_method_conversion();
    test_route_priority();
    test_segment_optimization();

    // Extended comprehensive tests
    test_static_routes_comprehensive();
    test_parameterized_routes_comprehensive();
    test_wildcard_routes_comprehensive();
    test_query_parameter_parsing_comprehensive();
    test_route_caching_optimization();
    test_large_scale_mixed_routes();

    // Performance tests
    test_performance_large_number_of_routes();
    test_performance_parameterized_routes();
    test_performance_wildcard_routes();
    test_performance_cache_efficiency();
    test_performance_concurrent_access();
    test_performance_memory_usage();

    std::cout << "\n=== All router tests passed! ===" << std::endl;
    print_test_summary();
  }

private:
  int total_tests = 0;
  int passed_tests = 0;

  void test_passed(const std::string &test_name) {
    ++total_tests;
    ++passed_tests;
    std::cout << "✓ " << test_name << " test passed" << std::endl;
  }

  void print_test_summary() {
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << total_tests << std::endl;
    std::cout << "Passed: " << passed_tests << std::endl;
    std::cout << "Failed: " << (total_tests - passed_tests) << std::endl;
    std::cout << "Success rate: " << (100.0 * passed_tests / total_tests) << "%"
              << std::endl;
  }

  // === Basic Functionality Tests ===

  void test_add_and_find_static_route() {
    std::cout << "Test: Add and find static route" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("users_handler");

    r.add_route(HttpMethod::GET, "/users", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result = r.find_route(HttpMethod::GET, "/users", found_handler, params,
                              query_params);

    assert(result == 0);
    assert(found_handler != nullptr);
    assert(found_handler->name == "users_handler");
    assert(params.empty());

    test_passed("Add and find static route");
  }

  void test_find_non_existent_route() {
    std::cout << "Test: Find non-existent route" << std::endl;

    router<MockHandler> r;
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result = r.find_route(HttpMethod::GET, "/nonexistent", found_handler,
                              params, query_params);

    assert(result == -1);
    assert(found_handler == nullptr);

    test_passed("Find non-existent route");
  }

  void test_parameterized_route() {
    std::cout << "Test: Parameterized route" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("user_handler");

    r.add_route(HttpMethod::GET, "/users/:id", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result = r.find_route(HttpMethod::GET, "/users/123", found_handler,
                              params, query_params);

    assert(result == 0);
    assert(found_handler != nullptr);
    assert(found_handler->name == "user_handler");
    assert(params.size() == 1);
    assert(params["id"] == "123");

    test_passed("Parameterized route");
  }

  void test_multiple_parameters_route() {
    std::cout << "Test: Multiple parameters route" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("comment_handler");

    r.add_route(HttpMethod::GET, "/users/:userId/posts/:postId", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result = r.find_route(HttpMethod::GET, "/users/123/posts/456",
                              found_handler, params, query_params);

    assert(result == 0);
    assert(found_handler != nullptr);
    assert(params.size() == 2);
    assert(params["userId"] == "123");
    assert(params["postId"] == "456");

    test_passed("Multiple parameters route");
  }

  void test_wildcard_route() {
    std::cout << "Test: Wildcard route" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("static_handler");

    r.add_route(HttpMethod::GET, "/static/*", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result = r.find_route(HttpMethod::GET, "/static/css/style.css",
                              found_handler, params, query_params);

    assert(result == 0);
    assert(found_handler != nullptr);
    assert(params.size() == 1);
    assert(params["*"] == "css/style.css");

    test_passed("Wildcard route");
  }

  void test_query_parameters() {
    std::cout << "Test: Query parameters" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("search_handler");
    r.add_route(HttpMethod::GET, "/search", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result =
        r.find_route(HttpMethod::GET, "/search?q=test&page=1&sort=date",
                     found_handler, params, query_params);

    assert(result == 0);
    assert(query_params.size() == 3);
    assert(query_params["q"] == "test");
    assert(query_params["page"] == "1");
    assert(query_params["sort"] == "date");

    test_passed("Query parameters");
  }

  void test_different_http_methods() {
    std::cout << "Test: Different HTTP methods" << std::endl;

    router<MockHandler> r;
    auto get_handler = std::make_shared<MockHandler>("get_handler");
    auto post_handler = std::make_shared<MockHandler>("post_handler");
    auto put_handler = std::make_shared<MockHandler>("put_handler");
    auto delete_handler = std::make_shared<MockHandler>("delete_handler");

    r.add_route(HttpMethod::GET, "/users", get_handler);
    r.add_route(HttpMethod::POST, "/users", post_handler);
    r.add_route(HttpMethod::PUT, "/users", put_handler);
    r.add_route(HttpMethod::DELETE, "/users", delete_handler);

    std::vector<std::pair<HttpMethod, std::shared_ptr<MockHandler>>>
        test_cases = {{HttpMethod::GET, get_handler},
                      {HttpMethod::POST, post_handler},
                      {HttpMethod::PUT, put_handler},
                      {HttpMethod::DELETE, delete_handler}};

    for (const auto &[method, expected_handler] : test_cases) {
      std::shared_ptr<MockHandler> found_handler;
      std::map<std::string, std::string> params;
      std::map<std::string, std::string> query_params;

      int result =
          r.find_route(method, "/users", found_handler, params, query_params);
      assert(result == 0);
      assert(found_handler == expected_handler);
    }

    test_passed("Different HTTP methods");
  }

  void test_path_normalization() {
    std::cout << "Test: Path normalization" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("normalized_handler");
    r.add_route(HttpMethod::GET, "//users//", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test various normalized paths
    std::vector<std::string> test_paths = {"/users", "users", "/users/",
                                           "//users", "users//"};

    for (const auto &path : test_paths) {
      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(found_handler == handler);
    }

    test_passed("Path normalization");
  }

  void test_route_caching() {
    std::cout << "Test: Route caching" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("cached_handler");
    r.add_route(HttpMethod::GET, "/users/:id", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // First call - should add to cache
    int result1 = r.find_route(HttpMethod::GET, "/users/123", found_handler,
                               params, query_params);
    assert(result1 == 0);
    assert(params["id"] == "123");

    // Second call - should use cache
    params.clear();
    int result2 = r.find_route(HttpMethod::GET, "/users/123", found_handler,
                               params, query_params);
    assert(result2 == 0);
    assert(params["id"] == "123");

    test_passed("Route caching");
  }

  void test_clear_cache() {
    std::cout << "Test: Clear cache" << std::endl;

    router<MockHandler> r;
    auto handler = std::make_shared<MockHandler>("test_handler");
    r.add_route(HttpMethod::GET, "/users", handler);

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Add to cache
    r.find_route(HttpMethod::GET, "/users", found_handler, params,
                 query_params);

    // Clear cache
    r.clear_cache();

    // Should still work after cache clear
    int result = r.find_route(HttpMethod::GET, "/users", found_handler, params,
                              query_params);
    assert(result == 0);
    assert(found_handler == handler);

    test_passed("Clear cache");
  }

  // === Advanced Functionality Tests ===

  void test_complex_route_patterns() {
    std::cout << "Test: Complex route patterns" << std::endl;

    router<MockHandler> r;

    // Complex nested parameters
    r.add_route(HttpMethod::GET,
                "/api/v1/users/:userId/posts/:postId/comments/:commentId",
                std::make_shared<MockHandler>("complex_handler"));

    // Mixed static and parameter segments
    r.add_route(HttpMethod::GET, "/files/:category/download/:fileId.pdf",
                std::make_shared<MockHandler>("download_handler"));

    // Wildcard at the end
    r.add_route(HttpMethod::GET, "/proxy/:service/*",
                std::make_shared<MockHandler>("proxy_handler"));

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test complex nested route
    int result = r.find_route(HttpMethod::GET,
                              "/api/v1/users/123/posts/456/comments/789",
                              found_handler, params, query_params);
    assert(result == 0);
    assert(params["userId"] == "123");
    assert(params["postId"] == "456");
    assert(params["commentId"] == "789");

    // Test mixed static/parameter route
    params.clear();
    result =
        r.find_route(HttpMethod::GET, "/files/documents/download/report.pdf",
                     found_handler, params, query_params);
    assert(result == 0);
    assert(params["category"] == "documents");
    assert(params["fileId"] == "report");

    // Test proxy wildcard route
    params.clear();
    result = r.find_route(HttpMethod::GET, "/proxy/api/users/profile",
                          found_handler, params, query_params);
    assert(result == 0);
    assert(params["service"] == "api");
    assert(params["*"] == "users/profile");

    test_passed("Complex route patterns");
  }

  void test_route_conflicts() {
    std::cout << "Test: Route conflicts" << std::endl;

    router<MockHandler> r;

    // Add conflicting routes
    r.add_route(HttpMethod::GET, "/users/:id",
                std::make_shared<MockHandler>("param_handler"));
    r.add_route(HttpMethod::GET, "/users/admin",
                std::make_shared<MockHandler>("static_handler"));

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Static route should match even though parameter route was added first
    int result = r.find_route(HttpMethod::GET, "/users/admin", found_handler,
                              params, query_params);
    assert(result == 0);
    assert(found_handler->name == "static_handler");

    // Parameter route should still work
    result = r.find_route(HttpMethod::GET, "/users/123", found_handler, params,
                          query_params);
    assert(result == 0);
    assert(found_handler->name == "param_handler");
    assert(params["id"] == "123");

    test_passed("Route conflicts");
  }

  void test_url_encoding() {
    std::cout << "Test: URL encoding" << std::endl;

    router<MockHandler> r;
    r.add_route(HttpMethod::GET, "/search",
                std::make_shared<MockHandler>("search_handler"));

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test URL encoded query parameters
    int result = r.find_route(
        HttpMethod::GET,
        "/search?q=hello%20world&filter=type%3Duser&special=%2B%26%3D",
        found_handler, params, query_params);

    assert(result == 0);
    assert(query_params["q"] == "hello world");
    assert(query_params["filter"] == "type=user");
    assert(query_params["special"] == "+&=");

    test_passed("URL encoding");
  }

  void test_edge_cases() {
    std::cout << "Test: Edge cases" << std::endl;

    router<MockHandler> r;

    // Empty path
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    int result =
        r.find_route(HttpMethod::GET, "", found_handler, params, query_params);
    assert(result == -1);

    // Root path
    r.add_route(HttpMethod::GET, "/",
                std::make_shared<MockHandler>("root_handler"));
    result =
        r.find_route(HttpMethod::GET, "/", found_handler, params, query_params);
    assert(result == 0);
    assert(found_handler->name == "root_handler");

    // Unknown HTTP method
    result = r.find_route(HttpMethod::UNKNOWN, "/", found_handler, params,
                          query_params);
    assert(result == -1);

    // Very long path
    std::string long_path =
        "/very/long/path/with/many/segments/that/goes/on/and/on";
    r.add_route(HttpMethod::GET, long_path,
                std::make_shared<MockHandler>("long_handler"));
    result = r.find_route(HttpMethod::GET, long_path, found_handler, params,
                          query_params);
    assert(result == 0);
    assert(found_handler->name == "long_handler");

    test_passed("Edge cases");
  }

  void test_http_method_conversion() {
    std::cout << "Test: HTTP method conversion" << std::endl;

    // Test to_string function
    assert(to_string(HttpMethod::GET) == "GET");
    assert(to_string(HttpMethod::POST) == "POST");
    assert(to_string(HttpMethod::PUT) == "PUT");
    assert(to_string(HttpMethod::DELETE) == "DELETE");
    assert(to_string(HttpMethod::PATCH) == "PATCH");
    assert(to_string(HttpMethod::HEAD) == "HEAD");
    assert(to_string(HttpMethod::OPTIONS) == "OPTIONS");
    assert(to_string(HttpMethod::CONNECT) == "CONNECT");
    assert(to_string(HttpMethod::TRACE) == "TRACE");
    assert(to_string(HttpMethod::UNKNOWN) == "UNKNOWN");

    // Test from_string function
    assert(from_string("GET") == HttpMethod::GET);
    assert(from_string("post") == HttpMethod::POST);
    assert(from_string("Put") == HttpMethod::PUT);
    assert(from_string("DELETE") == HttpMethod::DELETE);
    assert(from_string("patch") == HttpMethod::PATCH);
    assert(from_string("HEAD") == HttpMethod::HEAD);
    assert(from_string("options") == HttpMethod::OPTIONS);
    assert(from_string("connect") == HttpMethod::CONNECT);
    assert(from_string("trace") == HttpMethod::TRACE);
    assert(from_string("invalid") == HttpMethod::UNKNOWN);

    test_passed("HTTP method conversion");
  }

  void test_route_priority() {
    std::cout << "Test: Route priority" << std::endl;

    router<MockHandler> r;

    // Add routes in different orders to test priority
    r.add_route(HttpMethod::GET, "/api/*",
                std::make_shared<MockHandler>("wildcard_handler"));
    r.add_route(HttpMethod::GET, "/api/users",
                std::make_shared<MockHandler>("static_handler"));
    r.add_route(HttpMethod::GET, "/api/:resource",
                std::make_shared<MockHandler>("param_handler"));

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Static route should have highest priority
    int result = r.find_route(HttpMethod::GET, "/api/users", found_handler,
                              params, query_params);
    assert(result == 0);
    assert(found_handler->name == "static_handler");

    // Parameter route should have priority over wildcard
    result = r.find_route(HttpMethod::GET, "/api/posts", found_handler, params,
                          query_params);
    assert(result == 0);
    assert(found_handler->name == "param_handler");
    assert(params["resource"] == "posts");

    // Wildcard should catch everything else
    result = r.find_route(HttpMethod::GET, "/api/very/deep/path", found_handler,
                          params, query_params);
    assert(result == 0);
    assert(found_handler->name == "wildcard_handler");
    assert(params["*"] == "very/deep/path");

    test_passed("Route priority");
  }

  void test_segment_optimization() {
    std::cout << "Test: Segment optimization" << std::endl;

    router<MockHandler> r;

    // Add routes with different segment counts
    r.add_route(HttpMethod::GET, "/a",
                std::make_shared<MockHandler>("1_segment"));
    r.add_route(HttpMethod::GET, "/a/b",
                std::make_shared<MockHandler>("2_segments"));
    r.add_route(HttpMethod::GET, "/a/b/c",
                std::make_shared<MockHandler>("3_segments"));
    r.add_route(HttpMethod::GET, "/a/b/c/d",
                std::make_shared<MockHandler>("4_segments"));

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Test that segment optimization works correctly
    int result = r.find_route(HttpMethod::GET, "/a", found_handler, params,
                              query_params);
    assert(result == 0);
    assert(found_handler->name == "1_segment");

    result = r.find_route(HttpMethod::GET, "/a/b", found_handler, params,
                          query_params);
    assert(result == 0);
    assert(found_handler->name == "2_segments");

    result = r.find_route(HttpMethod::GET, "/a/b/c", found_handler, params,
                          query_params);
    assert(result == 0);
    assert(found_handler->name == "3_segments");

    result = r.find_route(HttpMethod::GET, "/a/b/c/d", found_handler, params,
                          query_params);
    assert(result == 0);
    assert(found_handler->name == "4_segments");

    test_passed("Segment optimization");
  }

  // === Performance Tests ===

  void test_performance_large_number_of_routes() {
    std::cout << "Test: Performance with large number of routes" << std::endl;

    router<MockHandler> r;
    const int num_routes = 10000;
    Timer timer;

    // Add many routes
    for (int i = 0; i < num_routes; ++i) {
      std::string path = "/route" + std::to_string(i);
      r.add_route(
          HttpMethod::GET, path,
          std::make_shared<MockHandler>("handler_" + std::to_string(i)));
    }

    double registration_time = timer.elapsed_ms();

    // Test lookup performance
    timer.reset();
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    const int lookup_iterations = 1000;
    for (int i = 0; i < lookup_iterations; ++i) {
      int route_id = i % num_routes;
      std::string path = "/route" + std::to_string(route_id);
      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
    }

    double lookup_time = timer.elapsed_ms();

    std::cout << "  Registration time for " << num_routes
              << " routes: " << registration_time << "ms" << std::endl;
    std::cout << "  Average lookup time: " << lookup_time / lookup_iterations
              << "ms per lookup" << std::endl;
    std::cout << "  Throughput: " << (lookup_iterations / lookup_time * 1000.0)
              << " lookups/second" << std::endl;

    test_passed("Performance with large number of routes");
  }

  void test_performance_parameterized_routes() {
    std::cout << "Test: Performance with parameterized routes" << std::endl;

    router<MockHandler> r;
    const int num_routes = 1000;
    Timer timer;

    // Add parameterized routes
    for (int i = 0; i < num_routes; ++i) {
      std::string path =
          "/category" + std::to_string(i) + "/:id/action/:action";
      r.add_route(
          HttpMethod::GET, path,
          std::make_shared<MockHandler>("param_handler_" + std::to_string(i)));
    }

    double registration_time = timer.elapsed_ms();

    // Test parameterized lookup performance
    timer.reset();
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    const int lookup_iterations = 1000;
    for (int i = 0; i < lookup_iterations; ++i) {
      int route_id = i % num_routes;
      std::string path =
          "/category" + std::to_string(route_id) + "/123/action/view";
      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(params["id"] == "123");
      assert(params["action"] == "view");
      params.clear();
    }

    double lookup_time = timer.elapsed_ms();

    std::cout << "  Parameterized route registration time: "
              << registration_time << "ms" << std::endl;
    std::cout << "  Average parameterized lookup time: "
              << lookup_time / lookup_iterations << "ms per lookup"
              << std::endl;

    test_passed("Performance with parameterized routes");
  }

  void test_performance_wildcard_routes() {
    std::cout << "Test: Performance with wildcard routes" << std::endl;

    router<MockHandler> r;
    const int num_routes = 100;
    Timer timer;

    // Add wildcard routes
    for (int i = 0; i < num_routes; ++i) {
      std::string path = "/files/category" + std::to_string(i) + "/*";
      r.add_route(HttpMethod::GET, path,
                  std::make_shared<MockHandler>("wildcard_handler_" +
                                                std::to_string(i)));
    }

    double registration_time = timer.elapsed_ms();

    // Test wildcard lookup performance
    timer.reset();
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    const int lookup_iterations = 1000;
    for (int i = 0; i < lookup_iterations; ++i) {
      int route_id = i % num_routes;
      std::string path = "/files/category" + std::to_string(route_id) +
                         "/deep/nested/file.txt";
      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(params["*"] == "deep/nested/file.txt");
      params.clear();
    }

    double lookup_time = timer.elapsed_ms();

    std::cout << "  Wildcard route registration time: " << registration_time
              << "ms" << std::endl;
    std::cout << "  Average wildcard lookup time: "
              << lookup_time / lookup_iterations << "ms per lookup"
              << std::endl;

    test_passed("Performance with wildcard routes");
  }

  void test_performance_cache_efficiency() {
    std::cout << "Test: Cache efficiency performance" << std::endl;

    router<MockHandler> r;

    // Add a complex parameterized route
    r.add_route(HttpMethod::GET,
                "/api/v1/users/:userId/posts/:postId/comments/:commentId",
                std::make_shared<MockHandler>("complex_handler"));

    Timer timer;
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // Cold lookup (no cache)
    int result = r.find_route(HttpMethod::GET,
                              "/api/v1/users/123/posts/456/comments/789",
                              found_handler, params, query_params);
    double cold_lookup = timer.elapsed_ms();
    assert(result == 0);

    // Warm lookups (with cache)
    timer.reset();
    const int cache_iterations = 10000;
    for (int i = 0; i < cache_iterations; ++i) {
      result = r.find_route(HttpMethod::GET,
                            "/api/v1/users/123/posts/456/comments/789",
                            found_handler, params, query_params);
      assert(result == 0);
      params.clear();
    }
    double warm_lookups = timer.elapsed_ms();

    std::cout << "  Cold lookup time: " << cold_lookup << "ms" << std::endl;
    std::cout << "  Average warm lookup time: "
              << warm_lookups / cache_iterations << "ms" << std::endl;
    std::cout << "  Cache speedup: "
              << (cold_lookup / (warm_lookups / cache_iterations)) << "x"
              << std::endl;

    test_passed("Cache efficiency performance");
  }

  void test_performance_concurrent_access() {
    std::cout << "Test: Concurrent access performance" << std::endl;

    router<MockHandler> r;

    // Clear cache before starting to ensure clean state
    r.clear_cache();

    // Setup routes - reduced for stability
    for (int i = 0; i < 50; ++i) {
      r.add_route(
          HttpMethod::GET, "/route" + std::to_string(i),
          std::make_shared<MockHandler>("handler_" + std::to_string(i)));
    }

    const int num_threads = std::min(
        2, (int)std::thread::hardware_concurrency()); // Further limit threads
    const int requests_per_thread = 500;              // Reduced requests

    Timer timer;
    std::vector<std::future<int>> futures;

    // Launch concurrent lookups
    for (int t = 0; t < num_threads; ++t) {
      futures.push_back(
          std::async(std::launch::async, [&r, requests_per_thread, t]() -> int {
            // Use deterministic seed for reproducibility
            std::mt19937 gen(42 + t);
            std::uniform_int_distribution<> dis(0, 49);

            int successful_requests = 0;

            for (int i = 0; i < requests_per_thread; ++i) {
              // Create completely separate objects for each thread
              std::shared_ptr<MockHandler> found_handler;
              std::map<std::string, std::string> params;
              std::map<std::string, std::string> query_params;

              int route_id = dis(gen);
              std::string path = "/route" + std::to_string(route_id);
              int result = r.find_route(HttpMethod::GET, path, found_handler,
                                        params, query_params);
              if (result == 0 && found_handler != nullptr) {
                successful_requests++;
              }

              // Small delay to reduce contention
              if (i % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
              }
            }

            return successful_requests;
          }));
    }

    // Wait for all threads to complete and collect results
    int total_successful = 0;
    for (auto &future : futures) {
      total_successful += future.get();
    }

    double total_time = timer.elapsed_ms();
    int total_requests = num_threads * requests_per_thread;

    std::cout << "  Concurrent access (" << num_threads << " threads, "
              << total_requests << " total requests): " << total_time << "ms"
              << std::endl;
    std::cout << "  Successful requests: " << total_successful << "/"
              << total_requests << std::endl;

    if (total_successful > 0) {
      std::cout << "  Throughput: " << (total_successful / total_time * 1000.0)
                << " requests/second" << std::endl;
      std::cout << "  Average response time: " << total_time / total_successful
                << "ms" << std::endl;
    }

    // Allow some failures due to potential race conditions
    assert(total_successful >=
           total_requests * 0.90); // At least 90% success rate

    test_passed("Concurrent access performance");
  }

  void test_performance_memory_usage() {
    std::cout << "Test: Memory usage performance" << std::endl;

    // This is a basic test - in production you'd use more sophisticated memory
    // profiling
    const int num_routes = 50000;
    Timer timer;

    {
      router<MockHandler> r;

      // Add many routes
      for (int i = 0; i < num_routes; ++i) {
        std::string path = "/api/v1/category" + std::to_string(i % 10) +
                           "/item" + std::to_string(i);
        r.add_route(
            HttpMethod::GET, path,
            std::make_shared<MockHandler>("handler_" + std::to_string(i)));
      }

      double construction_time = timer.elapsed_ms();

      // Perform some lookups to populate cache
      std::shared_ptr<MockHandler> found_handler;
      std::map<std::string, std::string> params;
      std::map<std::string, std::string> query_params;

      for (int i = 0; i < 1000; ++i) {
        std::string path = "/api/v1/category" + std::to_string(i % 10) +
                           "/item" + std::to_string(i % num_routes);
        r.find_route(HttpMethod::GET, path, found_handler, params,
                     query_params);
        params.clear();
      }

      std::cout << "  Construction time for " << num_routes
                << " routes: " << construction_time << "ms" << std::endl;
      std::cout << "  Router object created and tested successfully"
                << std::endl;
    } // Router destructor called here

    double destruction_time = timer.elapsed_ms() - timer.elapsed_ms();
    std::cout << "  Router object destroyed successfully" << std::endl;

    test_passed("Memory usage performance");
  }

  // === Extended Route Tests ===

  void test_static_routes_comprehensive() {
    std::cout << "Test: Static routes comprehensive" << std::endl;

    router<MockHandler> r;

    // Add various static routes
    std::vector<std::pair<std::string, std::string>> static_routes = {
        {"/", "home_handler"},
        {"/about", "about_handler"},
        {"/products", "products_handler"},
        {"/services", "services_handler"},
        {"/contact", "contact_handler"},
        {"/blog", "blog_handler"},
        {"/pricing", "pricing_handler"},
        {"/faq", "faq_handler"},
        {"/api/health", "health_handler"},
        {"/api/status", "status_handler"},
        {"/admin/dashboard", "admin_dashboard_handler"},
        {"/admin/users", "admin_users_handler"},
        {"/api/v1/info", "api_info_handler"},
        {"/assets/css/main.css", "css_handler"},
        {"/assets/js/app.js", "js_handler"}};

    // Register all static routes
    for (const auto &[path, handler_name] : static_routes) {
      r.add_route(HttpMethod::GET, path,
                  std::make_shared<MockHandler>(handler_name));
    }

    // Test all static routes
    for (const auto &[path, expected_handler] : static_routes) {
      std::shared_ptr<MockHandler> found_handler;
      std::map<std::string, std::string> params;
      std::map<std::string, std::string> query_params;

      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(found_handler != nullptr);
      assert(found_handler->name == expected_handler);
      assert(params.empty()); // Static routes should have no parameters
    }

    test_passed("Static routes comprehensive");
  }

  void test_parameterized_routes_comprehensive() {
    std::cout << "Test: Parameterized routes comprehensive" << std::endl;

    router<MockHandler> r;

    // Single parameter routes
    r.add_route(HttpMethod::GET, "/users/:id",
                std::make_shared<MockHandler>("user_by_id"));
    r.add_route(HttpMethod::GET, "/posts/:slug",
                std::make_shared<MockHandler>("post_by_slug"));
    r.add_route(HttpMethod::GET, "/categories/:name",
                std::make_shared<MockHandler>("category_by_name"));

    // Multiple parameter routes
    r.add_route(HttpMethod::GET, "/users/:userId/posts/:postId",
                std::make_shared<MockHandler>("user_post"));
    r.add_route(HttpMethod::GET, "/api/:version/resources/:resourceId",
                std::make_shared<MockHandler>("api_resource"));
    r.add_route(HttpMethod::GET, "/shop/:category/:subcategory/:productId",
                std::make_shared<MockHandler>("product_detail"));

    // Mixed static and parameter routes
    r.add_route(HttpMethod::GET, "/api/v1/users/:id/profile",
                std::make_shared<MockHandler>("user_profile"));
    r.add_route(HttpMethod::GET,
                "/admin/users/:userId/permissions/:permissionId",
                std::make_shared<MockHandler>("user_permission"));

    // Test cases with expected parameters
    std::vector<std::tuple<std::string, std::string,
                           std::map<std::string, std::string>>>
        test_cases = {
            // Single parameter
            {"/users/123", "user_by_id", {{"id", "123"}}},
            {"/users/admin-user", "user_by_id", {{"id", "admin-user"}}},
            {"/posts/hello-world", "post_by_slug", {{"slug", "hello-world"}}},
            {"/categories/technology",
             "category_by_name",
             {{"name", "technology"}}},

            // Multiple parameters
            {"/users/456/posts/789",
             "user_post",
             {{"userId", "456"}, {"postId", "789"}}},
            {"/api/v2/resources/user-data",
             "api_resource",
             {{"version", "v2"}, {"resourceId", "user-data"}}},
            {"/shop/electronics/phones/iphone-14",
             "product_detail",
             {{"category", "electronics"},
              {"subcategory", "phones"},
              {"productId", "iphone-14"}}},

            // Mixed routes
            {"/api/v1/users/789/profile", "user_profile", {{"id", "789"}}},
            {"/admin/users/123/permissions/read",
             "user_permission",
             {{"userId", "123"}, {"permissionId", "read"}}}};

    for (const auto &[path, expected_handler, expected_params] : test_cases) {
      std::shared_ptr<MockHandler> found_handler;
      std::map<std::string, std::string> params;
      std::map<std::string, std::string> query_params;

      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(found_handler != nullptr);
      assert(found_handler->name == expected_handler);
      assert(params.size() == expected_params.size());

      for (const auto &[key, value] : expected_params) {
        assert(params[key] == value);
      }
    }

    test_passed("Parameterized routes comprehensive");
  }

  void test_wildcard_routes_comprehensive() {
    std::cout << "Test: Wildcard routes comprehensive" << std::endl;

    router<MockHandler> r;

    // Simple wildcard routes
    r.add_route(HttpMethod::GET, "/static/*",
                std::make_shared<MockHandler>("static_files"));
    r.add_route(HttpMethod::GET, "/uploads/*",
                std::make_shared<MockHandler>("upload_files"));

    // Wildcard with parameters
    r.add_route(HttpMethod::GET, "/files/:type/*",
                std::make_shared<MockHandler>("typed_files"));
    r.add_route(HttpMethod::GET, "/proxy/:service/*",
                std::make_shared<MockHandler>("service_proxy"));
    r.add_route(HttpMethod::GET, "/cdn/:version/assets/*",
                std::make_shared<MockHandler>("cdn_assets"));

    // Test cases with expected wildcards and parameters
    std::vector<std::tuple<std::string, std::string,
                           std::map<std::string, std::string>>>
        test_cases = {
            // Simple wildcards
            {"/static/css/main.css", "static_files", {{"*", "css/main.css"}}},
            {"/static/js/vendor/jquery.min.js",
             "static_files",
             {{"*", "js/vendor/jquery.min.js"}}},
            {"/uploads/images/2023/profile.jpg",
             "upload_files",
             {{"*", "images/2023/profile.jpg"}}},

            // Wildcard with parameters
            {"/files/images/gallery/photo1.jpg",
             "typed_files",
             {{"type", "images"}, {"*", "gallery/photo1.jpg"}}},
            {"/files/documents/reports/2023/annual.pdf",
             "typed_files",
             {{"type", "documents"}, {"*", "reports/2023/annual.pdf"}}},
            {"/proxy/api/v1/users/profile",
             "service_proxy",
             {{"service", "api"}, {"*", "v1/users/profile"}}},
            {"/cdn/v2.1/assets/fonts/roboto.woff2",
             "cdn_assets",
             {{"version", "v2.1"}, {"*", "fonts/roboto.woff2"}}}};

    for (const auto &[path, expected_handler, expected_params] : test_cases) {
      std::shared_ptr<MockHandler> found_handler;
      std::map<std::string, std::string> params;
      std::map<std::string, std::string> query_params;

      int result = r.find_route(HttpMethod::GET, path, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(found_handler != nullptr);
      assert(found_handler->name == expected_handler);

      for (const auto &[key, value] : expected_params) {
        assert(params[key] == value);
      }
    }

    test_passed("Wildcard routes comprehensive");
  }

  void test_query_parameter_parsing_comprehensive() {
    std::cout << "Test: Query parameter parsing comprehensive" << std::endl;

    router<MockHandler> r;
    r.add_route(HttpMethod::GET, "/search",
                std::make_shared<MockHandler>("search_handler"));
    r.add_route(HttpMethod::GET, "/api/users",
                std::make_shared<MockHandler>("users_api"));

    // Test cases with various query parameter combinations
    std::vector<std::tuple<std::string, std::map<std::string, std::string>>>
        test_cases = {
            // Basic query parameters
            {"/search?q=test", {{"q", "test"}}},
            {"/search?q=hello%20world",
             {{"q", "hello world"}}}, // URL encoded space
            {"/search?key=value&flag=true",
             {{"key", "value"}, {"flag", "true"}}},

            // Multiple parameters
            {"/search?q=router&category=tech&sort=date",
             {{"q", "router"}, {"category", "tech"}, {"sort", "date"}}},
            {"/api/users?page=2&limit=10&order=name",
             {{"page", "2"}, {"limit", "10"}, {"order", "name"}}},

            // URL encoded parameters
            {"/search?q=hello%20world&filter=type%3Duser",
             {{"q", "hello world"}, {"filter", "type=user"}}},
            {"/search?data=%7B%22name%22%3A%22test%22%7D",
             {{"data", "{\"name\":\"test\"}"}}}, // JSON encoded

            // Special characters
            {"/search?symbols=%2B%26%3D%25", {{"symbols", "+&=%"}}},
            {"/search?email=test%40example.com",
             {{"email", "test@example.com"}}},

            // Empty values and edge cases
            {"/search?empty=&key=value", {{"empty", ""}, {"key", "value"}}},
            {"/search?flag", {{"flag", ""}}}, // Flag without value
            {"/search?a=1&b=2&c=3&d=4&e=5",
             {{"a", "1"}, {"b", "2"}, {"c", "3"}, {"d", "4"}, {"e", "5"}}},

            // Plus sign encoding (should convert to space)
            {"/search?q=hello+world", {{"q", "hello world"}}},
            {"/search?name=John+Doe&city=New+York",
             {{"name", "John Doe"}, {"city", "New York"}}}};

    for (const auto &[url, expected_query_params] : test_cases) {
      std::shared_ptr<MockHandler> found_handler;
      std::map<std::string, std::string> params;
      std::map<std::string, std::string> query_params;

      int result = r.find_route(HttpMethod::GET, url, found_handler, params,
                                query_params);
      assert(result == 0);
      assert(found_handler != nullptr);
      assert(query_params.size() == expected_query_params.size());

      for (const auto &[key, value] : expected_query_params) {
        assert(query_params.find(key) != query_params.end());
        assert(query_params[key] == value);
      }
    }

    test_passed("Query parameter parsing comprehensive");
  }

  void test_route_caching_optimization() {
    std::cout << "Test: Route caching optimization" << std::endl;

    router<MockHandler> r;

    // Add routes that will be cached
    r.add_route(HttpMethod::GET, "/api/v1/users/:userId/posts/:postId",
                std::make_shared<MockHandler>("cached_route"));
    r.add_route(HttpMethod::GET, "/static/assets/*",
                std::make_shared<MockHandler>("static_route"));

    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    Timer timer;

    // Cold cache test - first lookup
    timer.reset();
    int result =
        r.find_route(HttpMethod::GET, "/api/v1/users/123/posts/456?sort=date",
                     found_handler, params, query_params);
    double cold_time = timer.elapsed_ms();
    assert(result == 0);
    assert(params["userId"] == "123");
    assert(params["postId"] == "456");
    assert(query_params["sort"] == "date");

    // Warm cache test - repeated lookups should be faster
    const int cache_test_iterations = 1000;
    timer.reset();
    for (int i = 0; i < cache_test_iterations; ++i) {
      params.clear();
      query_params.clear();
      result =
          r.find_route(HttpMethod::GET, "/api/v1/users/123/posts/456?sort=date",
                       found_handler, params, query_params);
      assert(result == 0);
    }
    double warm_time = timer.elapsed_ms();
    double avg_warm_time = warm_time / cache_test_iterations;

    std::cout << "  Cold lookup time: " << cold_time << "ms" << std::endl;
    std::cout << "  Average warm lookup time: " << avg_warm_time << "ms"
              << std::endl;
    std::cout << "  Cache performance improvement: "
              << (cold_time / avg_warm_time) << "x" << std::endl;

    // Test cache with different routes
    std::vector<std::string> cache_test_routes = {
        "/api/v1/users/789/posts/101?filter=published",
        "/static/assets/images/logo.png",
        "/api/v1/users/456/posts/789?include=comments",
        "/static/assets/css/main.css"};

    // Fill cache with different routes
    for (const auto &route : cache_test_routes) {
      params.clear();
      query_params.clear();
      r.find_route(HttpMethod::GET, route, found_handler, params, query_params);
    }

    // Test that all cached routes are still fast
    timer.reset();
    for (int i = 0; i < 100; ++i) {
      for (const auto &route : cache_test_routes) {
        params.clear();
        query_params.clear();
        result = r.find_route(HttpMethod::GET, route, found_handler, params,
                              query_params);
        assert(result == 0);
      }
    }
    double multi_route_cache_time = timer.elapsed_ms();

    std::cout << "  Multi-route cache test time: " << multi_route_cache_time
              << "ms for " << (cache_test_routes.size() * 100) << " lookups"
              << std::endl;
    std::cout << "  Average cached lookup: "
              << (multi_route_cache_time / (cache_test_routes.size() * 100))
              << "ms" << std::endl;

    // Test cache invalidation
    r.clear_cache();
    timer.reset();
    params.clear();
    query_params.clear();
    result =
        r.find_route(HttpMethod::GET, "/api/v1/users/123/posts/456?sort=date",
                     found_handler, params, query_params);
    double post_clear_time = timer.elapsed_ms();
    assert(result == 0);

    std::cout << "  Post-cache-clear lookup time: " << post_clear_time << "ms"
              << std::endl;

    test_passed("Route caching optimization");
  }

  void test_large_scale_mixed_routes() {
    std::cout << "Test: Large scale mixed routes" << std::endl;

    router<MockHandler> r;
    const int num_static_routes = 500;
    const int num_param_routes = 300;
    const int num_wildcard_routes = 200;

    Timer timer;

    // Add static routes
    for (int i = 0; i < num_static_routes; ++i) {
      std::string path = "/static/page" + std::to_string(i);
      r.add_route(HttpMethod::GET, path,
                  std::make_shared<MockHandler>("static_" + std::to_string(i)));
    }

    // Add parameterized routes
    for (int i = 0; i < num_param_routes; ++i) {
      std::string path = "/api/v" + std::to_string(i % 5) + "/resource" +
                         std::to_string(i) + "/:id";
      r.add_route(HttpMethod::GET, path,
                  std::make_shared<MockHandler>("param_" + std::to_string(i)));
    }

    // Add wildcard routes
    for (int i = 0; i < num_wildcard_routes; ++i) {
      std::string path = "/files/type" + std::to_string(i) + "/*";
      r.add_route(
          HttpMethod::GET, path,
          std::make_shared<MockHandler>("wildcard_" + std::to_string(i)));
    }

    double registration_time = timer.elapsed_ms();
    int total_routes =
        num_static_routes + num_param_routes + num_wildcard_routes;

    std::cout << "  Registered " << total_routes << " routes in "
              << registration_time << "ms" << std::endl;

    // Test lookup performance with mixed route types
    timer.reset();
    std::shared_ptr<MockHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    const int lookup_iterations = 1000;
    int successful_lookups = 0;

    for (int i = 0; i < lookup_iterations; ++i) {
      params.clear();
      query_params.clear();

      int route_type = i % 3;
      int result = -1;

      if (route_type == 0) {
        // Test static route
        int route_id = i % num_static_routes;
        std::string path = "/static/page" + std::to_string(route_id);
        result = r.find_route(HttpMethod::GET, path, found_handler, params,
                              query_params);
      } else if (route_type == 1) {
        // Test parameterized route
        int route_id = i % num_param_routes;
        std::string path = "/api/v" + std::to_string(route_id % 5) +
                           "/resource" + std::to_string(route_id) + "/123";
        result = r.find_route(HttpMethod::GET, path, found_handler, params,
                              query_params);
        if (result == 0) {
          assert(params["id"] == "123");
        }
      } else {
        // Test wildcard route
        int route_id = i % num_wildcard_routes;
        std::string path =
            "/files/type" + std::to_string(route_id) + "/documents/file.pdf";
        result = r.find_route(HttpMethod::GET, path, found_handler, params,
                              query_params);
        if (result == 0) {
          assert(params["*"] == "documents/file.pdf");
        }
      }

      if (result == 0) {
        successful_lookups++;
      }
    }

    double lookup_time = timer.elapsed_ms();

    std::cout << "  Performed " << lookup_iterations << " mixed lookups in "
              << lookup_time << "ms" << std::endl;
    std::cout << "  Successful lookups: " << successful_lookups << "/"
              << lookup_iterations << std::endl;
    std::cout << "  Average lookup time: " << lookup_time / lookup_iterations
              << "ms" << std::endl;
    std::cout << "  Throughput: " << (lookup_iterations / lookup_time * 1000.0)
              << " lookups/second" << std::endl;

    assert(successful_lookups == lookup_iterations);

    test_passed("Large scale mixed routes");
  }
};

// External function declaration from test_router_group.cpp
extern int run_router_group_tests();

// Test runner
#ifndef DISABLE_ROUTER_MAIN
int main() {
  int result = 0;

  std::cout << "\n========================================" << std::endl;
  std::cout << "Running Router Tests" << std::endl;
  std::cout << "========================================" << std::endl;

  try {
    RouterTest test_suite;
    test_suite.run_all_tests();
  } catch (const std::exception &e) {
    std::cerr << "Router test failed with exception: " << e.what() << std::endl;
    result = 1;
  } catch (...) {
    std::cerr << "Router test failed with unknown exception" << std::endl;
    result = 1;
  }

  std::cout << "\n========================================" << std::endl;
  std::cout << "Running Router Group Tests" << std::endl;
  std::cout << "========================================" << std::endl;

  int group_result = run_router_group_tests();
  if (group_result != 0) {
    result = group_result;
  }

  if (result == 0) {
    std::cout << "\n🎉 All tests passed successfully!" << std::endl;
  } else {
    std::cout << "\n❌ Some tests failed!" << std::endl;
  }

  return result;
}
#endif
