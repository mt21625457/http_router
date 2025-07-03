/**
 * @file advanced_routing.cpp
 * @brief Advanced routing features demonstration
 * @author HTTP Router Team
 * @date 2025
 *
 * This example demonstrates advanced features of the HTTP Router:
 * - Complex route patterns
 * - Route priority and matching order
 * - URL decoding and special characters
 * - Performance optimization techniques
 * - Lambda expressions with captures
 * - Router groups and nested routing
 */

#include "router/router.hpp"
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace flc;

// Advanced handler types
class AdvancedHandler
{
public:
    AdvancedHandler(std::function<void(const std::map<std::string, std::string> &,
                                       const std::map<std::string, std::string> &)>
                        func)
        : func_(std::move(func))
    {
    }

    void operator()() const
    {
        std::cout << "Advanced handler called with no parameters" << std::endl;
    }

    void operator()(const std::map<std::string, std::string> &params,
                    const std::map<std::string, std::string> &query_params) const
    {
        if (func_) {
            func_(params, query_params);
        }
    }

private:
    std::function<void(const std::map<std::string, std::string> &,
                       const std::map<std::string, std::string> &)>
        func_;
};

class RequestContext
{
public:
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;

    RequestContext(const std::string &m, const std::string &p) : method(m), path(p) {}
};

class ResponseHandler
{
private:
    std::string name_;
    int status_code_;

public:
    ResponseHandler(const std::string &name, int status = 200) : name_(name), status_code_(status)
    {
    }

    void operator()() const
    {
        std::cout << "[" << status_code_ << "] " << name_ << " (no params)" << std::endl;
    }

    void operator()(const std::map<std::string, std::string> &params,
                    const std::map<std::string, std::string> &query_params)
    {
        std::cout << "[" << status_code_ << "] " << name_;

        if (!params.empty()) {
            std::cout << " | Path params: ";
            for (const auto &[key, value] : params) {
                std::cout << key << "=" << value << " ";
            }
        }

        if (!query_params.empty()) {
            std::cout << " | Query params: ";
            for (const auto &[key, value] : query_params) {
                std::cout << key << "=" << value << " ";
            }
        }
        std::cout << std::endl;
    }
};

void demonstrate_complex_patterns()
{
    std::cout << "\n=== Complex Route Patterns Demo ===\n";

    router<ResponseHandler> router_;

    // Add complex route patterns
    router_.add_route(HttpMethod::GET, "/api/v:version/users/:user_id/posts/:post_id",
                      ResponseHandler("GetUserPost"));

    router_.add_route(HttpMethod::GET, "/files/:category/:subcategory/*",
                      ResponseHandler("ServeFile"));

    router_.add_route(HttpMethod::GET, "/search/:type", ResponseHandler("SearchByType"));

    router_.add_route(HttpMethod::GET, "/admin/users/:id/permissions/:permission",
                      ResponseHandler("CheckPermission"));

    // Test complex patterns
    std::vector<std::string> test_paths = {
        "/api/v2/users/alice/posts/hello-world", "/files/images/thumbnails/photo.jpg",
        "/search/products?q=laptop&sort=price", "/admin/users/123/permissions/read",
        "/api/v1/users/bob/posts/my-blog-post?format=json"};

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_url_decoding()
{
    std::cout << "\n=== URL Decoding Demo ===\n";

    router<AdvancedHandler> router_;

    router_.add_route(HttpMethod::GET, "/search/:query",
                      AdvancedHandler([](const auto &params, const auto &query_params) {
                          std::cout << "Search query: '" << params.at("query") << "'\n";
                          for (const auto &[key, value] : query_params) {
                              std::cout << "  " << key << " = '" << value << "'\n";
                          }
                      }));

    router_.add_route(HttpMethod::GET, "/files/*",
                      AdvancedHandler([](const auto &params, const auto &query_params) {
                          std::cout << "File path: '" << params.at("*") << "'\n";
                      }));

    // Test URL encoding/decoding
    std::vector<std::string> encoded_paths = {
        "/search/hello%20world?page=1&filter=user%20data", "/files/documents/my%20file%20name.pdf",
        "/search/C%2B%2B%20programming?lang=en", "/files/path%2Fwith%2Fslashes/file.txt"};

    for (const auto &path : encoded_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing encoded: " << path << "\n -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
        std::cout << std::endl;
    }
}

void demonstrate_lambda_captures()
{
    std::cout << "\n=== Lambda Captures Demo ===\n";

    router<AdvancedHandler> router_;

    // Simulate application state
    std::string app_name = "MyApp";
    std::string version = "1.2.3";
    int request_count = 0;

    // Lambda with captures
    router_.add_route(HttpMethod::GET, "/status",
                      AdvancedHandler([&app_name, &version, &request_count](
                                          const auto &params, const auto &query_params) mutable {
                          ++request_count;
                          std::cout << "App: " << app_name << " v" << version
                                    << " | Requests served: " << request_count << std::endl;
                      }));

    // Lambda with shared state
    auto user_db = std::make_shared<std::map<std::string, std::string>>();
    (*user_db)["123"] = "Alice";
    (*user_db)["456"] = "Bob";
    (*user_db)["789"] = "Charlie";

    router_.add_route(HttpMethod::GET, "/users/:id",
                      AdvancedHandler([user_db](const auto &params, const auto &query_params) {
                          const auto &user_id = params.at("id");
                          auto it = user_db->find(user_id);
                          if (it != user_db->end()) {
                              std::cout << "User " << user_id << ": " << it->second << std::endl;
                          } else {
                              std::cout << "User " << user_id << " not found" << std::endl;
                          }
                      }));

    // Test lambda captures
    std::vector<std::string> test_paths = {
        "/status",    "/status",    "/status", // Multiple calls to see counter
        "/users/123", "/users/456", "/users/999"};

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_route_priorities()
{
    std::cout << "\n=== Route Priorities Demo ===\n";

    router<ResponseHandler> router_;

    // Add routes in specific order to test priority
    router_.add_route(HttpMethod::GET, "/api/*", ResponseHandler("CatchAllAPI"));
    router_.add_route(HttpMethod::GET, "/api/users", ResponseHandler("ListUsers"));
    router_.add_route(HttpMethod::GET, "/api/users/:id", ResponseHandler("GetUser"));
    router_.add_route(HttpMethod::GET, "/api/users/admin", ResponseHandler("AdminUsers"));
    router_.add_route(HttpMethod::GET, "/*", ResponseHandler("CatchAll"));

    std::cout << "Note: Router matches routes in the order they were added.\n";
    std::cout << "More specific routes should be added before general ones.\n\n";

    // Test route matching priority
    std::vector<std::string> test_paths = {
        "/api/users",       // Should match ListUsers
        "/api/users/123",   // Should match GetUser
        "/api/users/admin", // Should match GetUser (not AdminUsers, due to order)
        "/api/products",    // Should match CatchAllAPI
        "/other/path"       // Should match CatchAll
    };

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_performance_patterns()
{
    std::cout << "\n=== Performance Patterns Demo ===\n";

    router<AdvancedHandler> router_;

    // Add many routes to demonstrate caching
    const int num_routes = 1000;
    for (int i = 0; i < num_routes; ++i) {
        std::string path = "/api/resource" + std::to_string(i) + "/:id";
        router_.add_route(HttpMethod::GET, path,
                          AdvancedHandler([i](const auto &params, const auto &query_params) {
                              std::cout << "Resource " << i << " with ID: " << params.at("id")
                                        << std::endl;
                          }));
    }

    // Performance test
    std::vector<std::string> test_paths;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_routes - 1);

    // Generate random test paths
    for (int i = 0; i < 10; ++i) {
        int resource_id = dis(gen);
        std::string path =
            "/api/resource" + std::to_string(resource_id) + "/item" + std::to_string(i);
        test_paths.push_back(path);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        if (result.has_value()) {
            // Don't actually call the handler to focus on routing performance
            std::cout << "✓ Found route for: " << path << std::endl;
        } else {
            std::cout << "✗ No route for: " << path << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "\nPerformance test completed:\n";
    std::cout << "- Routes in router: " << num_routes << std::endl;
    std::cout << "- Test paths: " << test_paths.size() << std::endl;
    std::cout << "- Total time: " << duration.count() << " microseconds\n";
    std::cout << "- Average per lookup: " << (duration.count() / test_paths.size())
              << " microseconds\n";
}

// Middleware-like pattern using RAII
class RequestLogger
{
public:
    RequestLogger(const std::string &method, const std::string &path) : method_(method), path_(path)
    {
        start_time_ = std::chrono::high_resolution_clock::now();
        std::cout << "[REQUEST] " << method_ << " " << path_ << std::endl;
    }

    ~RequestLogger()
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        std::cout << "[RESPONSE] " << method_ << " " << path_ << " (" << duration.count() << "μs)"
                  << std::endl;
    }

private:
    std::string method_;
    std::string path_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

void demonstrate_middleware_pattern()
{
    std::cout << "\n=== Middleware Pattern Demo ===\n";

    router<AdvancedHandler> router_;

    router_.add_route(HttpMethod::GET, "/api/users/:id",
                      AdvancedHandler([](const auto &params, const auto &query_params) {
                          RequestLogger logger("GET", "/api/users/" + params.at("id"));

                          // Simulate some processing time
                          std::this_thread::sleep_for(std::chrono::milliseconds(10));

                          std::cout << "Processing user request for ID: " << params.at("id")
                                    << std::endl;
                      }));

    router_.add_route(HttpMethod::POST, "/api/users",
                      AdvancedHandler([](const auto &params, const auto &query_params) {
                          RequestLogger logger("POST", "/api/users");

                          std::cout << "Creating new user" << std::endl;
                      }));

    // Test middleware pattern
    std::vector<std::pair<HttpMethod, std::string>> test_cases = {
        {HttpMethod::GET, "/api/users/123"}, {HttpMethod::POST, "/api/users"}};

    for (const auto &[method, path] : test_cases) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(method, path, params, query_params);

        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found for " << static_cast<int>(method) << " " << path
                      << std::endl;
        }
        std::cout << std::endl;
    }
}

int main()
{
    std::cout << "HTTP Router - Advanced Routing Features\n";
    std::cout << "======================================\n";

    try {
        demonstrate_complex_patterns();
        demonstrate_url_decoding();
        demonstrate_lambda_captures();
        demonstrate_route_priorities();
        demonstrate_performance_patterns();
        demonstrate_middleware_pattern();

        std::cout << "\n=== All advanced demos completed! ===\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}