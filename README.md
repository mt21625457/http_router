# HTTP Router

A high-performance C++ HTTP router library with efficient path matching and caching mechanisms.

## Introduction

HTTP Router is a header-only C++ library designed for fast URL path matching with support for:

- Static routes (`/about`, `/products`)
- Parameterized routes (`/users/:id`, `/api/:version/resources/:resourceId`)
- Wildcard routes (`/static/*`, `/files/:type/*`)
- Query parameter parsing (`?key=value&flag=true`)
- Optimized route caching for improved performance

The library implements an intelligent hybrid matching algorithm that combines the benefits of hash tables, prefix trees, and LRU caching to achieve optimal performance for different route patterns.

## Requirements

- C++17 or higher
- Header-only (no build required)
- No external dependencies except for `tsl::htrie_map` for trie operations

## Features

- **High Performance**: Optimized for fast route matching
- **Flexible Routing**: Support for static, parameter, and wildcard routes
- **Query Parameter Parsing**: Built-in support for URL query parameters
- **Route Caching**: Implements LRU caching for frequently accessed routes
- **Header-only**: Easy to integrate with no build steps
- **Templated Design**: Works with any handler type
- **Memory Efficient**: Uses appropriate data structures based on route characteristics

## Usage Examples

### Basic Setup

```cpp
#include "http_router.hpp"
#include <iostream>
#include <functional>

// Define a simple handler type
using RouteHandler = std::function<void(const std::map<std::string, std::string>&)>;

int main() {
    // Create router instance
    http_router<RouteHandler> router;
    
    // Add static route
    router.add_route("/home", std::make_shared<RouteHandler>(
        [](const std::map<std::string, std::string>& params) {
            std::cout << "Home page" << std::endl;
        }
    ));
    
    // Add parameterized route
    router.add_route("/users/:id", std::make_shared<RouteHandler>(
        [](const std::map<std::string, std::string>& params) {
            std::cout << "User: " << params.at("id") << std::endl;
        }
    ));
    
    // Add wildcard route
    router.add_route("/files/*", std::make_shared<RouteHandler>(
        [](const std::map<std::string, std::string>& params) {
            std::cout << "File path: " << params.at("*") << std::endl;
        }
    ));
    
    // Process requests
    std::shared_ptr<RouteHandler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // Match a static route
    if (router.find_route("/home", handler, params, query_params) == 0) {
        (*handler)(params);  // Output: Home page
    }
    
    // Match a parameterized route
    if (router.find_route("/users/123", handler, params, query_params) == 0) {
        (*handler)(params);  // Output: User: 123
    }
    
    // Match a wildcard route with query parameters
    if (router.find_route("/files/documents/report.pdf?version=latest", handler, params, query_params) == 0) {
        (*handler)(params);  // Output: File path: documents/report.pdf
        std::cout << "Version: " << query_params["version"] << std::endl;  // Output: Version: latest
    }
    
    return 0;
}
```

### Custom Handler Class

```cpp
#include "http_router.hpp"
#include <iostream>

// Custom handler class
class RequestHandler {
public:
    RequestHandler(const std::string& name) : name_(name) {}
    
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
    http_router<RequestHandler> router;
    
    // Register routes
    router.add_route("/api/users", std::make_shared<RequestHandler>("ListUsers"));
    router.add_route("/api/users/:id", std::make_shared<RequestHandler>("GetUser"));
    router.add_route("/api/users/:id/posts", std::make_shared<RequestHandler>("GetUserPosts"));
    
    // Match a route
    std::shared_ptr<RequestHandler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // Process a request
    if (router.find_route("/api/users/42?fields=name,email", handler, params, query_params) == 0) {
        handler->handle(params, query_params);
    }
    
    return 0;
}
```

## Tests

The router comes with comprehensive test cases:

1. **`BasicFunctionalityTest`**: Tests basic static, parameter, and wildcard routes
2. **`CacheClearTest`**: Ensures cache clearing works properly
3. **`CachePerformanceTest`**: Measures performance gains from caching
4. **`LRUEvictionTest`**: Verifies the LRU cache eviction mechanism
5. **`RandomAccessPatternTest`**: Tests performance with real-world access patterns

To run the tests:

```bash
mkdir build && cd build
cmake ..
make
./http_router_test
```

## Performance

Route matching performance varies by route type:

| Route Type | Average Lookup Time | With Caching |
|------------|---------------------|--------------|
| Static     | ~0.5 µs             | ~0.1 µs      |
| Parameter  | ~2.0 µs             | ~0.1 µs      |
| Wildcard   | ~2.5 µs             | ~0.1 µs      |

*Times are approximate and will vary by hardware and route complexity.*

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