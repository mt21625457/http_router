# HTTP Router

A high-performance C++ HTTP router library with efficient path matching and caching mechanisms.

## Introduction

HTTP Router is a header-only C++ library designed for fast URL path matching with support for:

- Static routes (`/about`, `/products`)
- Parameterized routes (`/users/:id`, `/api/:version/resources/:resourceId`)
- Wildcard routes (`/static/*`, `/files/:type/*`)
- Query parameter parsing (`?key=value&flag=true`)
- Optimized route caching for improved performance
- Lambda expression support for modern C++ development
- Thread-safe concurrent access with SIMD optimizations

The library implements an intelligent hybrid matching algorithm that combines the benefits of hash tables, prefix trees, and LRU caching to achieve optimal performance for different route patterns.

## Requirements

- C++17 or higher (C++20 recommended)
- Header-only (no build required)
- No external dependencies except for `tsl::htrie_map` for trie operations
- CMake 3.10+ for building tests
- GoogleTest for running test suite

## Features

- **High Performance**: Optimized for fast route matching with SIMD optimizations
- **Flexible Routing**: Support for static, parameter, and wildcard routes
- **Query Parameter Parsing**: Built-in support for URL query parameters
- **Route Caching**: Implements LRU caching for frequently accessed routes
- **Header-only**: Easy to integrate with no build steps
- **Templated Design**: Works with any handler type
- **Memory Efficient**: Uses appropriate data structures based on route characteristics
- **Lambda Support**: Full support for C++ lambda expressions as route handlers
- **Thread Safety**: Concurrent access with atomic operations and lock-free structures
- **Object Pool**: Memory optimization with pre-allocated object pools
- **Fast Path Cache**: Specialized cache for common routes
- **HTTP Method Support**: Full support for all standard HTTP methods

## Usage Examples

### Basic Setup

```cpp
#include "router/router.hpp"
#include <iostream>
#include <functional>

using namespace flc;

// Define a simple handler type
using RouteHandler = std::function<void(const std::map<std::string, std::string>&)>;

int main() {
    // Create router instance
    router<RouteHandler> router_;
    
    // Add static route
    router_.add_route(HttpMethod::GET, "/home", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "Home page" << std::endl;
        }
    );
    
    // Add parameterized route
    router_.add_route(HttpMethod::GET, "/users/:id", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "User: " << params.at("id") << std::endl;
        }
    );
    
    // Add wildcard route
    router_.add_route(HttpMethod::GET, "/files/*", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "File path: " << params.at("*") << std::endl;
        }
    );
    
    // Process requests
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // Match a static route
    auto result = router_.find_route(HttpMethod::GET, "/home", params, query_params);
    if (result.has_value()) {
        result.value().get()(params);  // Output: Home page
    }
    
    // Match a parameterized route
    result = router_.find_route(HttpMethod::GET, "/users/123", params, query_params);
    if (result.has_value()) {
        result.value().get()(params);  // Output: User: 123
    }
    
    // Match a wildcard route with query parameters
    result = router_.find_route(HttpMethod::GET, "/files/documents/report.pdf?version=latest", params, query_params);
    if (result.has_value()) {
        result.value().get()(params);  // Output: File path: documents/report.pdf
        std::cout << "Version: " << query_params["version"] << std::endl;  // Output: Version: latest
    }
    
    return 0;
}
```

### Custom Handler Class

```cpp
#include "router/router.hpp"
#include <iostream>

using namespace flc;

// Custom handler class
class RequestHandler {
public:
    RequestHandler(const std::string& name) : name_(name) {}
    
    void operator()(const std::map<std::string, std::string>& params) {
        std::cout << "Handler '" << name_ << "' processing request" << std::endl;
        
        for (const auto& [key, value] : params) {
            std::cout << "  Param: " << key << " = " << value << std::endl;
        }
    }
    
    void handle(const std::map<std::string, std::string>& params,
                const std::map<std::string, std::string>& query_params) {
        std::cout << "Handler '" << name_ << "' processing request" << std::endl;
        
        for (const auto& [key, value] : params) {
            std::cout << "  Param: " << key << " = " << value << std::endl;
        }
        
        for (const auto& [key, value] : query_params) {
            std::cout << "  Query: " << key << " = " << value << std::endl;
        }
    }
    
private:
    std::string name_;
};

int main() {
    router<RequestHandler> router_;
    
    // Register routes
    router_.add_route(HttpMethod::GET, "/api/users", RequestHandler("ListUsers"));
    router_.add_route(HttpMethod::GET, "/api/users/:id", RequestHandler("GetUser"));
    router_.add_route(HttpMethod::GET, "/api/users/:id/posts", RequestHandler("GetUserPosts"));
    
    // Match a route
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // Process a request
    auto result = router_.find_route(HttpMethod::GET, "/api/users/42?fields=name,email", params, query_params);
    if (result.has_value()) {
        result.value().get().handle(params, query_params);
    }
    
    return 0;
}
```

### Lambda Expression Support

```cpp
#include "router/router.hpp"
#include <iostream>

using namespace flc;

int main() {
    // Router with lambda handlers
    router<std::function<void()>> router_;
    
    // Simple lambda handler
    router_.add_route(HttpMethod::GET, "/hello", 
        []() {
            std::cout << "Hello, World!" << std::endl;
        });
    
    // Router for handlers that need parameters
    router<std::function<void(const std::map<std::string, std::string>&)>> param_router;
    
    // Lambda with parameter extraction
    param_router.add_route(HttpMethod::GET, "/users/:id", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "User ID: " << params.at("id") << std::endl;
        });
    
    // Lambda with capture
    std::string server_name = "MyServer";
    router_.add_route(HttpMethod::GET, "/status", 
        [server_name]() {
            std::cout << "Server: " << server_name << " is running" << std::endl;
        });
    
    // Test the routes
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // Test simple lambda routes
    auto result = router_.find_route(HttpMethod::GET, "/hello", params, query_params);
    if (result.has_value()) {
        result.value().get()();
    }
    
    // Test parameterized lambda routes
    auto param_result = param_router.find_route(HttpMethod::GET, "/users/123", params, query_params);
    if (param_result.has_value()) {
        param_result.value().get()(params);
    }
    
    return 0;
}
```

### HTTP Methods Support

```cpp
#include "router/router.hpp"
#include <iostream>

using namespace flc;

int main() {
    router<std::function<void()>> router_;
    
    // Add routes with different HTTP methods
    router_.add_route(HttpMethod::GET, "/api/users", 
        []() { std::cout << "GET: List users" << std::endl; });
    
    router_.add_route(HttpMethod::POST, "/api/users", 
        []() { std::cout << "POST: Create user" << std::endl; });
    
    router_.add_route(HttpMethod::PUT, "/api/users/:id", 
        []() { std::cout << "PUT: Update user" << std::endl; });
    
    router_.add_route(HttpMethod::DELETE, "/api/users/:id", 
        []() { std::cout << "DELETE: Delete user" << std::endl; });
    
    // Find routes by method
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    auto result = router_.find_route(HttpMethod::GET, "/api/users", params, query_params);
    if (result.has_value()) {
        result.value().get()(); // Output: GET: List users
    }
    
    result = router_.find_route(HttpMethod::POST, "/api/users", params, query_params);
    if (result.has_value()) {
        result.value().get()(); // Output: POST: Create user
    }
    
    return 0;
}
```

## Tests

The router comes with comprehensive test cases across 12 test suites:

### Core Test Suites
1. **`HttpRouterTest`** (15 tests): Basic functionality including static, parameter, and wildcard routes
2. **`HttpRouterPerformanceTest`** (10 tests): Performance benchmarks for different route types
3. **`HybridRoutingTest`** (5 tests): Tests hybrid routing strategy with hash tables and tries
4. **`HttpRouterAdvTest`** (4 tests): Advanced features like URL decoding and LRU cache eviction
5. **`RouterStressTest`** (4 tests): Stress testing with large route sets and concurrent access

### Optimization Test Suites
6. **`RouterOptimizationIntegrationTest`** (15 tests): Integration tests for optimized routing
7. **`RouterOptimizationTest`** (29 tests): Comprehensive optimization feature testing
8. **`SimpleRouterTest`** (10 tests): Basic router functionality validation
9. **`BasicRouterTest`** (2 tests): Core router features

### Advanced Test Suites
10. **`LambdaContextTest`** (8 tests): Lambda expression support and performance
11. **`MassiveLambdaPerformanceTest`** (3 tests): Large-scale lambda route performance
12. **`MassiveLambdaPerformanceTestFixed`** (1 test): Fixed version of massive lambda tests

### Test Results Summary
- **Total Tests**: 106 tests across 12 test suites
- **Success Rate**: 100% (106/106 tests passed)
- **Total Test Time**: ~6 seconds
- **Thread Safety**: Verified with 16,000 concurrent operations
- **Large Scale**: Tested with 8,500 routes and 100,000 operations

### Running Tests

```bash
# Build the test suite
mkdir build && cd build
cmake ..
make

# Run all tests
./http_router_tests

# Or run tests with CMake
cd build
cmake ..
make test
```

## Performance Benchmarks

Based on comprehensive testing with the included test suite:

### Route Matching Performance
| Route Type | Average Lookup Time | With Caching | Throughput |
|------------|---------------------|--------------|------------|
| Static     | ~0.17 µs            | ~0.03 µs     | ~699K ops/sec |
| Parameter  | ~8.3 µs             | ~0.1 µs      | ~568K ops/sec |
| Wildcard   | ~94 µs              | ~0.1 µs      | ~100K ops/sec |

### Optimization Performance
| Operation | Time | Performance |
|-----------|------|-------------|
| Path Splitting | 0.0326 µs | Highly optimized |
| URL Decoding | 0.0511 µs | Safe and fast |
| Hex Conversion | 0.00022 ns | Ultra-fast |
| Lambda Handlers | 0.177 µs | Minimal overhead |

### Large Scale Performance
- **8,500 routes**: Successfully tested with massive route sets
- **100,000 operations**: Handled with 30%+ cache hit rate
- **Multi-threading**: 10 threads with 99.9%+ success rate
- **Memory efficiency**: Object pooling for optimized memory usage

### Thread Safety Results
- **16,000 concurrent operations**: 100% success rate
- **10 threads**: Average 13-16 µs per operation
- **Cache hit rate**: 30-31% under concurrent load

*Performance measurements taken on Linux with modern hardware. Results may vary by system configuration.*

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

MIT License

Copyright (c) 2025 MT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. 