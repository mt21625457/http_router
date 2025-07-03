/**
 * @file basic_usage.cpp
 * @brief Basic usage example of the HTTP Router library
 * @author HTTP Router Team
 * @date 2025
 *
 * This example demonstrates the basic functionality of the HTTP Router:
 * - Creating a router instance
 * - Adding static, parameterized, and wildcard routes
 * - Finding routes and extracting parameters
 * - Query parameter parsing
 */

#include "router/router.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <string>

using namespace co;

// Simple handler function type
using SimpleHandler = std::function<void()>;

// Handler that can access path parameters
class ParamHandler
{
public:
    ParamHandler(std::function<void(const std::map<std::string, std::string> &)> func)
        : func_(std::move(func))
    {
    }

    void operator()() const { std::cout << "Param handler called with no parameters" << std::endl; }

    void operator()(const std::map<std::string, std::string> &params) const
    {
        if (func_) {
            func_(params);
        }
    }

private:
    std::function<void(const std::map<std::string, std::string> &)> func_;
};

// Handler that can access both path and query parameters
class FullHandler
{
public:
    FullHandler(std::function<void(const std::map<std::string, std::string> &,
                                   const std::map<std::string, std::string> &)>
                    func)
        : func_(std::move(func))
    {
    }

    void operator()() const { std::cout << "Full handler called with no parameters" << std::endl; }

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

void demonstrate_static_routes()
{
    std::cout << "\n=== Static Routes Demo ===\n";

    router<SimpleHandler> router_;

    // Add static routes
    router_.add_route(HttpMethod::GET, "/", []() { std::cout << "Welcome to the home page!\n"; });

    router_.add_route(HttpMethod::GET, "/about", []() { std::cout << "About page\n"; });

    router_.add_route(HttpMethod::GET, "/contact", []() { std::cout << "Contact page\n"; });

    // Test static routes
    std::vector<std::string> test_paths = {"/", "/about", "/contact", "/notfound"};

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing path: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()();
        } else {
            std::cout << "Route not found\n";
        }
    }
}

void demonstrate_parameterized_routes()
{
    std::cout << "\n=== Parameterized Routes Demo ===\n";

    router<ParamHandler> router_;

    // Add parameterized routes
    router_.add_route(HttpMethod::GET, "/users/:id", ParamHandler([](const auto &params) {
                          std::cout << "User profile for ID: " << params.at("id") << "\n";
                      }));

    router_.add_route(
        HttpMethod::GET, "/users/:id/posts/:post_id", ParamHandler([](const auto &params) {
            std::cout << "Post " << params.at("post_id") << " by user " << params.at("id") << "\n";
        }));

    router_.add_route(HttpMethod::GET, "/api/:version/users/:user_id",
                      ParamHandler([](const auto &params) {
                          std::cout << "API v" << params.at("version")
                                    << " - User: " << params.at("user_id") << "\n";
                      }));

    // Test parameterized routes
    std::vector<std::string> test_paths = {"/users/123", "/users/456/posts/789",
                                           "/api/v2/users/alice", "/users/invalid/extra/path"};

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing path: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params);
        } else {
            std::cout << "Route not found\n";
        }
    }
}

void demonstrate_wildcard_routes()
{
    std::cout << "\n=== Wildcard Routes Demo ===\n";

    router<ParamHandler> router_;

    // Add wildcard routes
    router_.add_route(HttpMethod::GET, "/static/*", ParamHandler([](const auto &params) {
                          std::cout << "Serving static file: " << params.at("*") << "\n";
                      }));

    router_.add_route(HttpMethod::GET, "/files/:type/*", ParamHandler([](const auto &params) {
                          std::cout << "File type: " << params.at("type")
                                    << ", path: " << params.at("*") << "\n";
                      }));

    // Test wildcard routes
    std::vector<std::string> test_paths = {"/static/css/style.css", "/static/js/app.js",
                                           "/files/images/photo.jpg",
                                           "/files/documents/report.pdf"};

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing path: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params);
        } else {
            std::cout << "Route not found\n";
        }
    }
}

void demonstrate_query_parameters()
{
    std::cout << "\n=== Query Parameters Demo ===\n";

    // For this demo, we'll use a custom handler that can access query params
    router<FullHandler> router_;

    router_.add_route(HttpMethod::GET, "/search",
                      FullHandler([](const auto &params, const auto &query_params) {
                          std::cout << "Search results";
                          for (const auto &[key, value] : query_params) {
                              std::cout << " | " << key << "=" << value;
                          }
                          std::cout << "\n";
                      }));

    router_.add_route(HttpMethod::GET, "/users/:id",
                      FullHandler([](const auto &params, const auto &query_params) {
                          std::cout << "User " << params.at("id");
                          for (const auto &[key, value] : query_params) {
                              std::cout << " | " << key << "=" << value;
                          }
                          std::cout << "\n";
                      }));

    // Test query parameters
    std::vector<std::string> test_paths = {"/search?q=router&sort=name&limit=10",
                                           "/users/123?format=json&include=posts",
                                           "/search?q=hello%20world&page=1"};

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "Testing path: " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "Route not found\n";
        }
    }
}

// Custom handler class example
class ApiHandler
{
public:
    ApiHandler(const std::string &endpoint) : endpoint_(endpoint) {}

    void operator()() const
    {
        std::cout << "API Endpoint: " << endpoint_ << " (no params)" << std::endl;
    }

    void operator()(const std::map<std::string, std::string> &params)
    {
        std::cout << "API Endpoint: " << endpoint_;
        if (!params.empty()) {
            std::cout << " | Params: ";
            for (const auto &[key, value] : params) {
                std::cout << key << "=" << value << " ";
            }
        }
        std::cout << "\n";
    }

private:
    std::string endpoint_;
};

void demonstrate_custom_handlers()
{
    std::cout << "\n=== Custom Handler Classes Demo ===\n";

    router<ApiHandler> router_;

    // Add routes with custom handler objects
    router_.add_route(HttpMethod::GET, "/api/users", ApiHandler("ListUsers"));
    router_.add_route(HttpMethod::GET, "/api/users/:id", ApiHandler("GetUser"));
    router_.add_route(HttpMethod::POST, "/api/users", ApiHandler("CreateUser"));
    router_.add_route(HttpMethod::PUT, "/api/users/:id", ApiHandler("UpdateUser"));
    router_.add_route(HttpMethod::DELETE, "/api/users/:id", ApiHandler("DeleteUser"));

    // Test different HTTP methods and paths
    std::vector<std::pair<HttpMethod, std::string>> test_cases = {
        {HttpMethod::GET, "/api/users"},
        {HttpMethod::GET, "/api/users/42"},
        {HttpMethod::POST, "/api/users"},
        {HttpMethod::PUT, "/api/users/42"},
        {HttpMethod::DELETE, "/api/users/42"}};

    for (const auto &[method, path] : test_cases) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(method, path, params, query_params);

        std::cout << "Testing " << static_cast<int>(method) << " " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params);
        } else {
            std::cout << "Route not found\n";
        }
    }
}

int main()
{
    std::cout << "HTTP Router - Basic Usage Examples\n";
    std::cout << "==================================\n";

    try {
        demonstrate_static_routes();
        demonstrate_parameterized_routes();
        demonstrate_wildcard_routes();
        demonstrate_query_parameters();
        demonstrate_custom_handlers();

        std::cout << "\n=== All demos completed successfully! ===\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}