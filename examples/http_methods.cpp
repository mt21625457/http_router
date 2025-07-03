/**
 * @file http_methods.cpp
 * @brief HTTP methods demonstration for RESTful API design
 * @author HTTP Router Team
 * @date 2025
 *
 * This example demonstrates how to handle different HTTP methods:
 * - GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS
 * - RESTful API design patterns
 * - Method-specific route handling
 * - CRUD operations simulation
 */

#include "router/router.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace flc;

// Handler that knows about HTTP methods
class RestApiHandler
{
private:
    std::string resource_;
    std::string operation_;

public:
    RestApiHandler(const std::string &resource, const std::string &operation)
        : resource_(resource), operation_(operation)
    {
    }

    void operator()() const
    {
        std::cout << "[" << resource_ << "] " << operation_ << " (no params)" << std::endl;
    }

    void operator()(const std::map<std::string, std::string> &params,
                    const std::map<std::string, std::string> &query_params)
    {
        std::cout << "[" << resource_ << "] " << operation_;

        if (!params.empty()) {
            std::cout << " | Params: ";
            for (const auto &[key, value] : params) {
                std::cout << key << "=" << value << " ";
            }
        }

        if (!query_params.empty()) {
            std::cout << " | Query: ";
            for (const auto &[key, value] : query_params) {
                std::cout << key << "=" << value << " ";
            }
        }
        std::cout << std::endl;
    }
};

// Simple CRUD simulation
class CrudHandler
{
public:
    void operator()() const
    {
        // Empty implementation for simple handlers
    }
};

// 使用函数适配器包装处理器
using HandlerAdapter = std::function<void()>;

void demonstrate_basic_http_methods()
{
    std::cout << "\n=== Basic HTTP Methods Demo ===\n";

    router<RestApiHandler> router_;

    // 直接使用RestApiHandler，不需要适配器
    router_.add_route(HttpMethod::GET, "/users", RestApiHandler("Users", "List all users"));
    router_.add_route(HttpMethod::POST, "/users", RestApiHandler("Users", "Create new user"));
    router_.add_route(HttpMethod::GET, "/users/:id", RestApiHandler("Users", "Get user by ID"));
    router_.add_route(HttpMethod::PUT, "/users/:id",
                      RestApiHandler("Users", "Update user (full replace)"));
    router_.add_route(HttpMethod::PATCH, "/users/:id",
                      RestApiHandler("Users", "Update user (partial)"));
    router_.add_route(HttpMethod::DELETE, "/users/:id", RestApiHandler("Users", "Delete user"));

    // Product resource endpoints
    router_.add_route(HttpMethod::GET, "/products", RestApiHandler("Products", "List products"));
    router_.add_route(HttpMethod::POST, "/products", RestApiHandler("Products", "Create product"));
    router_.add_route(HttpMethod::GET, "/products/:id", RestApiHandler("Products", "Get product"));
    router_.add_route(HttpMethod::PUT, "/products/:id",
                      RestApiHandler("Products", "Update product"));
    router_.add_route(HttpMethod::DELETE, "/products/:id",
                      RestApiHandler("Products", "Delete product"));

    // Test all HTTP methods
    std::vector<std::pair<HttpMethod, std::string>> test_cases = {
        {HttpMethod::GET, "/users"},
        {HttpMethod::POST, "/users"},
        {HttpMethod::GET, "/users/123"},
        {HttpMethod::PUT, "/users/123"},
        {HttpMethod::PATCH, "/users/123"},
        {HttpMethod::DELETE, "/users/123"},
        {HttpMethod::GET, "/products?category=electronics&sort=price"},
        {HttpMethod::POST, "/products"},
        {HttpMethod::GET, "/products/456"},
        {HttpMethod::PUT, "/products/456"},
        {HttpMethod::DELETE, "/products/456"}};

    for (const auto &[method, path] : test_cases) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(method, path, params, query_params);

        // Convert method enum to string for display
        std::string method_str;
        switch (method) {
        case HttpMethod::GET:
            method_str = "GET";
            break;
        case HttpMethod::POST:
            method_str = "POST";
            break;
        case HttpMethod::PUT:
            method_str = "PUT";
            break;
        case HttpMethod::DELETE:
            method_str = "DELETE";
            break;
        case HttpMethod::PATCH:
            method_str = "PATCH";
            break;
        case HttpMethod::HEAD:
            method_str = "HEAD";
            break;
        case HttpMethod::OPTIONS:
            method_str = "OPTIONS";
            break;
        default:
            method_str = "UNKNOWN";
            break;
        }

        std::cout << method_str << " " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_nested_resources()
{
    std::cout << "\n=== Nested Resources Demo ===\n";

    router<RestApiHandler> router_;

    // User posts (nested resource)
    router_.add_route(HttpMethod::GET, "/users/:user_id/posts",
                      RestApiHandler("UserPosts", "List user's posts"));
    router_.add_route(HttpMethod::POST, "/users/:user_id/posts",
                      RestApiHandler("UserPosts", "Create post for user"));
    router_.add_route(HttpMethod::GET, "/users/:user_id/posts/:post_id",
                      RestApiHandler("UserPosts", "Get specific post"));
    router_.add_route(HttpMethod::PUT, "/users/:user_id/posts/:post_id",
                      RestApiHandler("UserPosts", "Update user's post"));
    router_.add_route(HttpMethod::DELETE, "/users/:user_id/posts/:post_id",
                      RestApiHandler("UserPosts", "Delete user's post"));

    // User comments (deeply nested)
    router_.add_route(HttpMethod::GET, "/users/:user_id/posts/:post_id/comments",
                      RestApiHandler("PostComments", "List post comments"));
    router_.add_route(HttpMethod::POST, "/users/:user_id/posts/:post_id/comments",
                      RestApiHandler("PostComments", "Add comment to post"));
    router_.add_route(HttpMethod::DELETE, "/users/:user_id/posts/:post_id/comments/:comment_id",
                      RestApiHandler("PostComments", "Delete comment"));

    // Test nested resources
    std::vector<std::pair<HttpMethod, std::string>> test_cases = {
        {HttpMethod::GET, "/users/alice/posts"},
        {HttpMethod::POST, "/users/alice/posts"},
        {HttpMethod::GET, "/users/alice/posts/my-first-post"},
        {HttpMethod::PUT, "/users/alice/posts/my-first-post"},
        {HttpMethod::DELETE, "/users/alice/posts/my-first-post"},
        {HttpMethod::GET, "/users/bob/posts/hello-world/comments"},
        {HttpMethod::POST, "/users/bob/posts/hello-world/comments"},
        {HttpMethod::DELETE, "/users/bob/posts/hello-world/comments/comment123"}};

    for (const auto &[method, path] : test_cases) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(method, path, params, query_params);

        std::string method_str;
        switch (method) {
        case HttpMethod::GET:
            method_str = "GET";
            break;
        case HttpMethod::POST:
            method_str = "POST";
            break;
        case HttpMethod::PUT:
            method_str = "PUT";
            break;
        case HttpMethod::DELETE:
            method_str = "DELETE";
            break;
        default:
            method_str = "UNKNOWN";
            break;
        }

        std::cout << method_str << " " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_api_versioning()
{
    std::cout << "\n=== API Versioning Demo ===\n";

    router<RestApiHandler> router_;

    // API v1
    router_.add_route(HttpMethod::GET, "/api/v1/users",
                      RestApiHandler("APIv1", "List users (old format)"));
    router_.add_route(HttpMethod::GET, "/api/v1/users/:id",
                      RestApiHandler("APIv1", "Get user (basic info)"));

    // API v2 - enhanced features
    router_.add_route(HttpMethod::GET, "/api/v2/users",
                      RestApiHandler("APIv2", "List users (enhanced format)"));
    router_.add_route(HttpMethod::GET, "/api/v2/users/:id",
                      RestApiHandler("APIv2", "Get user (detailed info)"));
    router_.add_route(HttpMethod::GET, "/api/v2/users/:id/profile",
                      RestApiHandler("APIv2", "Get user profile (new feature)"));

    // Dynamic versioning
    router_.add_route(HttpMethod::GET, "/api/:version/products",
                      RestApiHandler("Products", "List products (version-aware)"));
    router_.add_route(HttpMethod::GET, "/api/:version/products/:id",
                      RestApiHandler("Products", "Get product (version-aware)"));

    // Test different API versions
    std::vector<std::pair<HttpMethod, std::string>> test_cases = {
        {HttpMethod::GET, "/api/v1/users"},
        {HttpMethod::GET, "/api/v1/users/123"},
        {HttpMethod::GET, "/api/v2/users"},
        {HttpMethod::GET, "/api/v2/users/123"},
        {HttpMethod::GET, "/api/v2/users/123/profile"},
        {HttpMethod::GET, "/api/v1/products"},
        {HttpMethod::GET, "/api/v2/products"},
        {HttpMethod::GET, "/api/v3/products/456"} // Future version
    };

    for (const auto &[method, path] : test_cases) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(method, path, params, query_params);

        std::cout << "GET " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_content_negotiation()
{
    std::cout << "\n=== Content Negotiation Simulation ===\n";

    // 创建一个支持无参数调用的处理器类型
    class ContentHandler
    {
    public:
        ContentHandler(std::function<void(const std::map<std::string, std::string> &,
                                          const std::map<std::string, std::string> &)>
                           func)
            : func_(std::move(func))
        {
        }

        void operator()() const
        {
            // 无参数调用时的默认行为
            std::cout << "Content handler called with no parameters" << std::endl;
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

    router<ContentHandler> router_;

    // Same endpoint, different content handling based on query params
    router_.add_route(HttpMethod::GET, "/api/users/:id",
                      ContentHandler([](const auto &params, const auto &query_params) {
                          std::string user_id = params.at("id");
                          std::string format = "json"; // default

                          auto format_it = query_params.find("format");
                          if (format_it != query_params.end()) {
                              format = format_it->second;
                          }

                          std::cout << "Serving user " << user_id << " in " << format << " format";

                          if (format == "json") {
                              std::cout << " -> {\"id\":\"" << user_id << "\",\"name\":\"User "
                                        << user_id << "\"}";
                          } else if (format == "xml") {
                              std::cout << " -> <user><id>" << user_id << "</id><name>User "
                                        << user_id << "</name></user>";
                          } else if (format == "csv") {
                              std::cout << " -> id,name\\n" << user_id << ",User " << user_id;
                          } else {
                              std::cout << " -> Unsupported format: " << format;
                          }
                          std::cout << std::endl;
                      }));

    // Test content negotiation
    std::vector<std::string> test_paths = {
        "/api/users/123",             // Default JSON
        "/api/users/123?format=json", // Explicit JSON
        "/api/users/123?format=xml",  // XML format
        "/api/users/123?format=csv",  // CSV format
        "/api/users/123?format=yaml"  // Unsupported format
    };

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        std::cout << "GET " << path << "\n  ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "No route found\n";
        }
    }
}

void demonstrate_method_not_allowed()
{
    std::cout << "\n=== Method Not Allowed Demo ===\n";

    router<RestApiHandler> router_;

    // Only allow GET and POST for /api/public
    router_.add_route(HttpMethod::GET, "/api/public", RestApiHandler("Public", "Get public data"));
    router_.add_route(HttpMethod::POST, "/api/public",
                      RestApiHandler("Public", "Submit public data"));

    // Only allow GET for /api/readonly
    router_.add_route(HttpMethod::GET, "/api/readonly",
                      RestApiHandler("ReadOnly", "Get read-only data"));

    // Test allowed and disallowed methods
    std::vector<std::pair<HttpMethod, std::string>> test_cases = {
        // Allowed methods
        {HttpMethod::GET, "/api/public"},
        {HttpMethod::POST, "/api/public"},
        {HttpMethod::GET, "/api/readonly"},

        // Disallowed methods (should not find routes)
        {HttpMethod::DELETE, "/api/public"},
        {HttpMethod::PUT, "/api/public"},
        {HttpMethod::POST, "/api/readonly"},
        {HttpMethod::DELETE, "/api/readonly"}};

    for (const auto &[method, path] : test_cases) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(method, path, params, query_params);

        std::string method_str;
        switch (method) {
        case HttpMethod::GET:
            method_str = "GET";
            break;
        case HttpMethod::POST:
            method_str = "POST";
            break;
        case HttpMethod::PUT:
            method_str = "PUT";
            break;
        case HttpMethod::DELETE:
            method_str = "DELETE";
            break;
        default:
            method_str = "UNKNOWN";
            break;
        }

        std::cout << method_str << " " << path << " -> ";
        if (result.has_value()) {
            result.value().get()(params, query_params);
        } else {
            std::cout << "405 Method Not Allowed\n";
        }
    }
}

int main()
{
    std::cout << "HTTP Router - HTTP Methods and RESTful API Examples\n";
    std::cout << "==================================================\n";

    try {
        demonstrate_basic_http_methods();
        demonstrate_nested_resources();
        demonstrate_api_versioning();
        demonstrate_content_negotiation();
        demonstrate_method_not_allowed();

        std::cout << "\n=== All HTTP methods demos completed! ===\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}