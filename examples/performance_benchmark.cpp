/**
 * @file performance_benchmark.cpp
 * @brief Performance benchmarking and optimization examples
 * @author HTTP Router Team
 * @date 2025
 *
 * This example demonstrates:
 * - Performance benchmarking techniques
 * - Route lookup timing comparisons
 * - Cache effectiveness measurement
 * - Memory usage optimization
 * - Thread safety performance
 * - Large-scale routing performance
 */

#include "router/router.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace co;

class BenchmarkTimer
{
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::string name_;

public:
    BenchmarkTimer(const std::string &name) : name_(name)
    {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    ~BenchmarkTimer()
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        std::cout << "[TIMER] " << name_ << ": " << duration.count() << " μs" << std::endl;
    }

    long long elapsed_microseconds() const
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_)
            .count();
    }
};

// Simple handler for performance testing
class FastHandler
{
public:
    void operator()() const
    {
        // Minimal operation for performance testing
    }

    void operator()(const std::map<std::string, std::string> &) const
    {
        // Minimal operation for performance testing
    }
};

void benchmark_route_types()
{
    std::cout << "\n=== Route Type Performance Benchmark ===\n";

    router<FastHandler> router_;

    // Add different types of routes
    const int num_routes = 1000;

    // Static routes
    for (int i = 0; i < num_routes; ++i) {
        std::string path = "/static/route" + std::to_string(i);
        router_.add_route(HttpMethod::GET, path, FastHandler{});
    }

    // Parameterized routes
    for (int i = 0; i < num_routes; ++i) {
        std::string path = "/param" + std::to_string(i) + "/:id";
        router_.add_route(HttpMethod::GET, path, FastHandler{});
    }

    // Wildcard routes
    for (int i = 0; i < num_routes; ++i) {
        std::string path = "/wildcard" + std::to_string(i) + "/*";
        router_.add_route(HttpMethod::GET, path, FastHandler{});
    }

    std::cout << "Router loaded with " << (num_routes * 3) << " routes\n";

    const int num_tests = 10000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_routes - 1);

    // Benchmark static routes
    {
        BenchmarkTimer timer("Static Routes (" + std::to_string(num_tests) + " lookups)");
        for (int i = 0; i < num_tests; ++i) {
            int route_id = dis(gen);
            std::string path = "/static/route" + std::to_string(route_id);

            std::map<std::string, std::string> params, query_params;
            auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

            // Ensure the compiler doesn't optimize away the result
            volatile bool found = result.has_value();
            (void)found;
        }
    }

    // Benchmark parameterized routes
    {
        BenchmarkTimer timer("Parameterized Routes (" + std::to_string(num_tests) + " lookups)");
        for (int i = 0; i < num_tests; ++i) {
            int route_id = dis(gen);
            std::string path = "/param" + std::to_string(route_id) + "/item123";

            std::map<std::string, std::string> params, query_params;
            auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

            volatile bool found = result.has_value();
            (void)found;
        }
    }

    // Benchmark wildcard routes
    {
        BenchmarkTimer timer("Wildcard Routes (" + std::to_string(num_tests) + " lookups)");
        for (int i = 0; i < num_tests; ++i) {
            int route_id = dis(gen);
            std::string path = "/wildcard" + std::to_string(route_id) + "/some/deep/path.txt";

            std::map<std::string, std::string> params, query_params;
            auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

            volatile bool found = result.has_value();
            (void)found;
        }
    }
}

void benchmark_cache_effectiveness()
{
    std::cout << "\n=== Cache Effectiveness Benchmark ===\n";

    router<FastHandler> router_;

    // Add routes
    const int num_routes = 5000;
    for (int i = 0; i < num_routes; ++i) {
        std::string path = "/api/resource" + std::to_string(i) + "/:id";
        router_.add_route(HttpMethod::GET, path, FastHandler{});
    }

    // Create test paths - some repeated to test cache
    std::vector<std::string> test_paths;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_routes - 1);

    // Generate paths with 70% cache hit rate simulation
    const int num_tests = 50000;
    for (int i = 0; i < num_tests; ++i) {
        if (i < num_tests * 0.7) {
            // Repeat some paths for cache hits
            int route_id = dis(gen) % 100; // Limited set for cache hits
            test_paths.push_back("/api/resource" + std::to_string(route_id) + "/item" +
                                 std::to_string(i % 10));
        } else {
            // Random paths for cache misses
            int route_id = dis(gen);
            test_paths.push_back("/api/resource" + std::to_string(route_id) + "/item" +
                                 std::to_string(i));
        }
    }

    // Shuffle to avoid sequential access patterns
    std::shuffle(test_paths.begin(), test_paths.end(), gen);

    std::cout << "Testing with " << test_paths.size() << " path lookups...\n";

    int found_count = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto &path : test_paths) {
        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        if (result.has_value()) {
            ++found_count;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    std::cout << "Results:\n";
    std::cout << "  Total time: " << total_time.count() << " μs\n";
    std::cout << "  Average per lookup: " << (total_time.count() / test_paths.size()) << " μs\n";
    std::cout << "  Routes found: " << found_count << "/" << test_paths.size() << "\n";
    std::cout << "  Throughput: " << (test_paths.size() * 1000000LL / total_time.count())
              << " lookups/sec\n";
}

void benchmark_memory_usage()
{
    std::cout << "\n=== Memory Usage Benchmark ===\n";

    // This is a simplified memory usage test
    // In a real application, you'd use more sophisticated memory profiling tools

    auto measure_routes = [](int num_routes) {
        router<FastHandler> router_;

        auto start_time = std::chrono::high_resolution_clock::now();

        // Add routes
        for (int i = 0; i < num_routes; ++i) {
            std::string path = "/api/v1/resource" + std::to_string(i) + "/:id/action/:action";
            router_.add_route(HttpMethod::GET, path, FastHandler{});
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto creation_time =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        // Test lookup performance
        std::string test_path =
            "/api/v1/resource" + std::to_string(num_routes / 2) + "/123/action/update";
        std::map<std::string, std::string> params, query_params;

        start_time = std::chrono::high_resolution_clock::now();
        auto result = router_.find_route(HttpMethod::GET, test_path, params, query_params);
        end_time = std::chrono::high_resolution_clock::now();
        auto lookup_time =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        std::cout << "  " << std::setw(6) << num_routes << " routes: "
                  << "creation=" << std::setw(6) << creation_time.count() << "μs, "
                  << "lookup=" << std::setw(4) << lookup_time.count() << "ns\n";

        return result.has_value();
    };

    std::cout << "Measuring route creation and lookup times:\n";
    std::vector<int> route_counts = {100, 500, 1000, 2500, 5000, 10000, 25000};

    for (int count : route_counts) {
        measure_routes(count);
    }
}

void benchmark_concurrent_access()
{
    std::cout << "\n=== Concurrent Access Benchmark ===\n";

    router<FastHandler> router_;

    // Add routes
    const int num_routes = 10000;
    for (int i = 0; i < num_routes; ++i) {
        std::string path = "/concurrent/resource" + std::to_string(i) + "/:id";
        router_.add_route(HttpMethod::GET, path, FastHandler{});
    }

    std::cout << "Testing concurrent access with " << num_routes << " routes...\n";

    const int num_threads = std::thread::hardware_concurrency();
    const int lookups_per_thread = 100000;

    std::atomic<long long> total_operations{0};
    std::atomic<long long> successful_operations{0};
    std::vector<std::thread> threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Launch worker threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&router_, t, lookups_per_thread, &total_operations,
                              &successful_operations]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 9999);

            for (int i = 0; i < lookups_per_thread; ++i) {
                int route_id = dis(gen);
                std::string path =
                    "/concurrent/resource" + std::to_string(route_id) + "/item" + std::to_string(i);

                std::map<std::string, std::string> params, query_params;
                auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

                total_operations.fetch_add(1, std::memory_order_relaxed);
                if (result.has_value()) {
                    successful_operations.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Concurrent access results:\n";
    std::cout << "  Threads: " << num_threads << "\n";
    std::cout << "  Total operations: " << total_operations.load() << "\n";
    std::cout << "  Successful operations: " << successful_operations.load() << "\n";
    std::cout << "  Success rate: "
              << (100.0 * successful_operations.load() / total_operations.load()) << "%\n";
    std::cout << "  Total time: " << total_time.count() << " ms\n";
    std::cout << "  Throughput: " << (total_operations.load() * 1000LL / total_time.count())
              << " ops/sec\n";
    std::cout << "  Average per operation: "
              << (total_time.count() * 1000.0 / total_operations.load()) << " μs\n";
}

void benchmark_query_parameter_parsing()
{
    std::cout << "\n=== Query Parameter Parsing Benchmark ===\n";

    router<FastHandler> router_;

    router_.add_route(HttpMethod::GET, "/search", FastHandler{});
    router_.add_route(HttpMethod::GET, "/api/users/:id", FastHandler{});

    // Generate test URLs with varying query parameter complexity
    std::vector<std::string> test_urls = {
        "/search?q=simple", "/search?q=hello&sort=name&order=asc",
        "/search?q=complex%20query&page=1&limit=50&sort=created_at&order=desc&filter=active",
        "/api/users/123?include=posts&include=comments&format=json&fields=id,name,email",
        "/search?q=url%20encoded%20string&category=electronics&price_min=100&price_max=500&brand="
        "apple&brand=samsung"};

    const int iterations = 100000;

    for (const auto &base_url : test_urls) {
        auto start_time = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            std::map<std::string, std::string> params, query_params;
            auto result = router_.find_route(HttpMethod::GET, base_url, params, query_params);

            // Ensure compiler doesn't optimize away the parsing
            volatile size_t param_count = query_params.size();
            (void)param_count;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        std::cout << "URL: " << base_url.substr(0, 50) << (base_url.length() > 50 ? "..." : "")
                  << "\n";
        std::cout << "  Total time: " << duration.count() << " μs\n";
        std::cout << "  Per operation: " << (duration.count() / iterations) << " μs\n\n";
    }
}

void benchmark_large_scale_routing()
{
    std::cout << "\n=== Large Scale Routing Benchmark ===\n";

    router<FastHandler> router_;

    // Simulate a large application with many routes
    const int num_routes = 50000;
    std::cout << "Creating " << num_routes << " routes...\n";

    auto creation_start = std::chrono::high_resolution_clock::now();

    // Add various route patterns
    for (int i = 0; i < num_routes; ++i) {
        if (i % 4 == 0) {
            // Static routes
            router_.add_route(HttpMethod::GET, "/static/page" + std::to_string(i), FastHandler{});
        } else if (i % 4 == 1) {
            // Single parameter routes
            router_.add_route(HttpMethod::GET, "/users/" + std::to_string(i) + "/:id",
                              FastHandler{});
        } else if (i % 4 == 2) {
            // Multiple parameter routes
            router_.add_route(HttpMethod::GET,
                              "/api/v" + std::to_string(i % 5) + "/res" + std::to_string(i) +
                                  "/:id/sub/:sub_id",
                              FastHandler{});
        } else {
            // Wildcard routes
            router_.add_route(HttpMethod::GET, "/files/" + std::to_string(i) + "/*", FastHandler{});
        }
    }

    auto creation_end = std::chrono::high_resolution_clock::now();
    auto creation_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(creation_end - creation_start);

    std::cout << "Route creation completed in " << creation_time.count() << " ms\n";

    // Performance test with random lookups
    const int num_lookups = 1000000;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_routes - 1);

    std::cout << "Performing " << num_lookups << " random lookups...\n";

    int found_count = 0;
    auto lookup_start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_lookups; ++i) {
        int route_id = dis(gen);
        std::string path;

        if (route_id % 4 == 0) {
            path = "/static/page" + std::to_string(route_id);
        } else if (route_id % 4 == 1) {
            path = "/users/" + std::to_string(route_id) + "/item123";
        } else if (route_id % 4 == 2) {
            path = "/api/v" + std::to_string(route_id % 5) + "/res" + std::to_string(route_id) +
                   "/456/sub/789";
        } else {
            path = "/files/" + std::to_string(route_id) + "/documents/file.pdf";
        }

        std::map<std::string, std::string> params, query_params;
        auto result = router_.find_route(HttpMethod::GET, path, params, query_params);

        if (result.has_value()) {
            ++found_count;
        }
    }

    auto lookup_end = std::chrono::high_resolution_clock::now();
    auto lookup_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(lookup_end - lookup_start);

    std::cout << "Large scale benchmark results:\n";
    std::cout << "  Routes in router: " << num_routes << "\n";
    std::cout << "  Lookup operations: " << num_lookups << "\n";
    std::cout << "  Routes found: " << found_count << "\n";
    std::cout << "  Total lookup time: " << lookup_time.count() << " ms\n";
    std::cout << "  Average per lookup: " << (lookup_time.count() * 1000.0 / num_lookups)
              << " μs\n";
    std::cout << "  Throughput: " << (num_lookups * 1000LL / lookup_time.count())
              << " lookups/sec\n";
}

int main()
{
    std::cout << "HTTP Router - Performance Benchmarks\n";
    std::cout << "====================================\n";

    try {
        benchmark_route_types();
        benchmark_cache_effectiveness();
        benchmark_memory_usage();
        benchmark_concurrent_access();
        benchmark_query_parameter_parsing();
        benchmark_large_scale_routing();

        std::cout << "\n=== All performance benchmarks completed! ===\n";
        std::cout << "\nNote: Performance results depend on hardware, compiler optimizations,\n";
        std::cout << "and system load. Run multiple times for consistent measurements.\n";

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}