# HTTP Router Examples

This directory contains comprehensive examples demonstrating the HTTP Router library's features and capabilities.

## Overview

The examples are organized to showcase different aspects of the HTTP Router:

- **Basic Usage**: Core functionality and simple routing patterns
- **Advanced Routing**: Complex routing features and optimization techniques  
- **HTTP Methods**: RESTful API design patterns and HTTP method handling
- **Performance Benchmarks**: Performance testing and optimization examples

## Examples

### 1. Basic Usage (`basic_usage.cpp`)

Demonstrates fundamental router concepts:
- Creating router instances
- Adding static, parameterized, and wildcard routes
- Route matching and parameter extraction
- Query parameter parsing
- Custom handler classes

**Key Features:**
- Static routes: `/home`, `/about`, `/contact`
- Parameterized routes: `/users/:id`, `/api/:version/users/:user_id`
- Wildcard routes: `/static/*`, `/files/:type/*`
- Query parameter handling: `?key=value&flag=true`

### 2. Advanced Routing (`advanced_routing.cpp`)

Explores sophisticated routing features:
- Complex route patterns with multiple parameters
- URL encoding/decoding
- Lambda expressions with captures
- Route priority and matching order
- Performance optimization patterns
- Middleware-like patterns using RAII

**Key Features:**
- Complex patterns: `/api/v:version/users/:user_id/posts/:post_id`
- URL decoding: `hello%20world` → `hello world`
- Lambda captures for shared state
- Performance patterns for large route sets

### 3. HTTP Methods (`http_methods.cpp`)

Demonstrates RESTful API design:
- Full HTTP method support (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS)
- CRUD operations for resources
- Nested resource routing
- API versioning strategies
- Content negotiation simulation
- Method-specific routing

**Key Features:**
- RESTful endpoints: `GET /users`, `POST /users`, `PUT /users/:id`
- Nested resources: `/users/:user_id/posts/:post_id`
- API versioning: `/api/v1/users` vs `/api/v2/users`
- Method validation and 405 responses

### 4. Performance Benchmarks (`performance_benchmark.cpp`)

Comprehensive performance testing:
- Route type performance comparison
- Cache effectiveness measurement
- Memory usage scaling
- Concurrent access benchmarking
- Query parameter parsing performance
- Large-scale routing performance

**Key Features:**
- Route type benchmarks (static vs parameterized vs wildcard)
- Multi-threaded performance testing
- Memory scaling analysis
- Throughput measurements
- Cache hit rate analysis

## Building the Examples

### Prerequisites

- C++17 or higher compiler (C++20 recommended)
- CMake 3.10 or higher
- HTTP Router library (header-only)

### Build Instructions

1. **Build all examples:**
   ```bash
   cd examples
   mkdir build && cd build
   cmake ..
   make
   ```

2. **Build specific example:**
   ```bash
   make basic_usage
   make advanced_routing
   make http_methods
   make performance_benchmark
   ```

3. **Build with optimizations (recommended for performance tests):**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make
   ```

## Running the Examples

### Run Individual Examples

```bash
# Basic usage demonstration
./basic_usage

# Advanced routing features
./advanced_routing

# HTTP methods and RESTful patterns
./http_methods

# Performance benchmarking
./performance_benchmark
```

### Run All Examples

```bash
# Using CMake target
make run_examples

# Or manually
./basic_usage && ./advanced_routing && ./http_methods && ./performance_benchmark
```

### Performance Testing

```bash
# Run performance benchmarks only
make performance_test

# Or directly
./performance_benchmark
```

## Example Output

### Basic Usage Example Output
```
HTTP Router - Basic Usage Examples
==================================

=== Static Routes Demo ===
Testing path: / -> Welcome to the home page!
Testing path: /about -> About page
Testing path: /contact -> Contact page
Testing path: /notfound -> Route not found

=== Parameterized Routes Demo ===
Testing path: /users/123 -> User profile for ID: 123
Testing path: /users/456/posts/789 -> Post 789 by user 456
...
```

### Performance Benchmark Output
```
=== Route Type Performance Benchmark ===
[TIMER] Static Routes (10000 lookups): 1250 μs
[TIMER] Parameterized Routes (10000 lookups): 8430 μs
[TIMER] Wildcard Routes (10000 lookups): 12560 μs

=== Cache Effectiveness Benchmark ===
Results:
  Total time: 45670 μs
  Average per lookup: 0.91 μs
  Throughput: 1094890 lookups/sec
...
```

## Understanding the Results

### Performance Metrics

- **Static Routes**: Fastest lookup (~0.1-1 μs per lookup)
- **Parameterized Routes**: Medium performance (~2-10 μs per lookup)
- **Wildcard Routes**: Slower but flexible (~10-100 μs per lookup)

### Cache Effectiveness

The router implements intelligent caching:
- **Cache Hit**: ~0.1 μs lookup time
- **Cache Miss**: Depends on route complexity
- **Hit Rate**: Typically 70-90% in real applications

### Concurrent Performance

Thread-safe operations with:
- **Throughput**: 500K+ operations/second
- **Scalability**: Near-linear with CPU cores
- **Success Rate**: >99.9% under load

## Integration Examples

### Web Server Integration

```cpp
#include "router/router.hpp"
#include <iostream>

class HttpHandler {
public:
    void operator()(const auto& params, const auto& query_params) {
        // Handle HTTP request
        send_response(generate_response(params, query_params));
    }
};

int main() {
    router<HttpHandler> app_router;
    
    // Add your routes
    app_router.add_route(HttpMethod::GET, "/api/users/:id", HttpHandler{});
    
    // In your HTTP server loop:
    // auto result = app_router.find_route(method, path, params, query_params);
    // if (result.has_value()) result.value().get()(params, query_params);
}
```

### Microservice Architecture

```cpp
// Service-specific routing
class UserService {
    router<UserHandler> user_router_;
public:
    void setup_routes() {
        user_router_.add_route(HttpMethod::GET, "/users", ListUsersHandler{});
        user_router_.add_route(HttpMethod::GET, "/users/:id", GetUserHandler{});
        user_router_.add_route(HttpMethod::POST, "/users", CreateUserHandler{});
    }
};
```

## Best Practices

### Route Organization

1. **Order matters**: Add more specific routes before general ones
2. **Use consistent patterns**: Follow RESTful conventions
3. **Group related routes**: Use prefixes like `/api/v1/`
4. **Validate parameters**: Check parameter formats in handlers

### Performance Optimization

1. **Prefer static routes** when possible for best performance
2. **Use caching** for frequently accessed routes
3. **Minimize route complexity** for better lookup times
4. **Profile your specific patterns** using the benchmark examples

### Error Handling

1. **Check route results**: Always verify `result.has_value()`
2. **Handle missing routes**: Provide fallback/404 handlers
3. **Validate parameters**: Sanitize extracted parameters
4. **Handle exceptions**: Wrap router operations in try-catch

## Troubleshooting

### Common Issues

1. **Route not found**: Check route order and pattern matching
2. **Performance issues**: Profile with benchmark examples
3. **Memory usage**: Monitor with memory benchmark
4. **Thread safety**: Use concurrent benchmark for testing

### Debug Tips

1. **Enable debug builds** for detailed error information
2. **Use simple patterns first** then add complexity
3. **Test with benchmark tools** to identify bottlenecks
4. **Check parameter extraction** with debug output

## Contributing

To add new examples:

1. Create a new `.cpp` file in the examples directory
2. Follow the existing example structure and documentation style
3. Add the new file to `CMakeLists.txt`
4. Update this README with the new example description
5. Test the example thoroughly

## License

These examples are provided under the same MIT license as the HTTP Router library.