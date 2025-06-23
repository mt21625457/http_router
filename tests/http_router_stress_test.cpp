#include "http_router.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <random>

// Handler for testing purposes
class StressTestDummyHandler
{
public:
    StressTestDummyHandler() = default;
    explicit StressTestDummyHandler(int id) : id_(id) {}
    int id() const { return id_; }

private:
    int id_ = 0;
};

// Test suite for stress testing
class HttpRouterStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        router = std::make_unique<http_router<StressTestDummyHandler>>();
    }

    std::unique_ptr<http_router<StressTestDummyHandler>> router;
};

// Performance test with a million routes
TEST_F(HttpRouterStressTest, MillionRoutesPerformance) {
    const int NUM_ROUTES = 1000000;
    // Set lookup count to be equal to the cache size to properly test a hot cache scenario
    const int NUM_LOOKUPS = 1000; // Corresponds to MAX_CACHE_SIZE in http_router.hpp

    // Use a single handler to save memory, as the handler itself is not the focus of this test.
    auto handler = std::make_shared<StressTestDummyHandler>(1);

    std::cout << "\n[ STRESS TEST ] Starting to add " << NUM_ROUTES << " routes..." << std::endl;
    auto start_add = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ROUTES; ++i) {
        std::string path;
        // Mix of route types to test the hybrid strategy at scale
        switch (i % 4) {
            case 0:
                path = "/short" + std::to_string(i); // Expected to be stored in the hash map
                break;
            case 1:
                path = "/long/path/for/trie/test/" + std::to_string(i); // Expected to be stored in the Trie
                break;
            case 2:
                path = "/param/:id/user/" + std::to_string(i); // Parameterized route
                break;
            case 3:
                path = "/wildcard/" + std::to_string(i) + "/*"; // Wildcard route
                break;
        }
        router->add_route(HttpMethod::GET, path, handler);

        // Add progress indicator every 1%
        if ((i + 1) % (NUM_ROUTES / 100) == 0) {
            std::cout << "[ STRESS TEST ] ... " << (static_cast<double>(i + 1) / NUM_ROUTES) * 100 << "% of routes added." << std::endl;
        }
    }

    auto end_add = std::chrono::high_resolution_clock::now();
    auto add_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_add - start_add).count();
    std::cout << "[ STRESS TEST ] Finished adding routes in " << add_duration << " ms." << std::endl;
    if (NUM_ROUTES > 0) {
        std::cout << "[ STRESS TEST ] Average time per route addition: " << static_cast<double>(add_duration * 1000) / NUM_ROUTES << " us." << std::endl;
    }

    // Prepare lookup paths
    std::vector<std::string> lookup_paths;
    lookup_paths.reserve(NUM_LOOKUPS);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, NUM_ROUTES - 1);

    for (int i = 0; i < NUM_LOOKUPS; ++i) {
        int route_idx = distrib(gen);
        std::string path;
        switch (route_idx % 4) {
            case 0:
                path = "/short" + std::to_string(route_idx);
                break;
            case 1:
                path = "/long/path/for/trie/test/" + std::to_string(route_idx);
                break;
            case 2:
                path = "/param/123/user/" + std::to_string(route_idx);
                break;
            case 3:
                path = "/wildcard/" + std::to_string(route_idx) + "/some/path";
                break;
        }
        lookup_paths.push_back(path);
    }

    std::shared_ptr<StressTestDummyHandler> found_handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;

    // First run to populate the cache
    std::cout << "[ STRESS TEST ] Starting " << NUM_LOOKUPS << " lookups (no cache / populating cache)..." << std::endl;
    auto start_lookup_nocache = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < lookup_paths.size(); ++i) {
        router->find_route(HttpMethod::GET, lookup_paths[i], found_handler, params, query_params);
        // Add progress indicator every 1%
        if ((i + 1) % (NUM_LOOKUPS / 100) == 0) {
             std::cout << "[ STRESS TEST ] ... " << (static_cast<double>(i + 1) / NUM_LOOKUPS) * 100 << "% of non-cached lookups completed." << std::endl;
        }
    }
    auto end_lookup_nocache = std::chrono::high_resolution_clock::now();
    auto lookup_nocache_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_lookup_nocache - start_lookup_nocache).count();
    std::cout << "[ STRESS TEST ] Finished non-cached lookups in " << lookup_nocache_duration << " ms." << std::endl;
    if (NUM_LOOKUPS > 0) {
        std::cout << "[ STRESS TEST ] Average time per lookup (no cache): " << static_cast<double>(lookup_nocache_duration * 1000) / NUM_LOOKUPS << " us." << std::endl;
    }

    // Second run to measure performance with a hot cache
    std::cout << "[ STRESS TEST ] Starting " << NUM_LOOKUPS << " lookups (with hot cache)..." << std::endl;
    auto start_lookup_cache = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < lookup_paths.size(); ++i) {
        router->find_route(HttpMethod::GET, lookup_paths[i], found_handler, params, query_params);
        // Add progress indicator every 1%
        if ((i + 1) % (NUM_LOOKUPS / 100) == 0) {
             std::cout << "[ STRESS TEST ] ... " << (static_cast<double>(i + 1) / NUM_LOOKUPS) * 100 << "% of cached lookups completed." << std::endl;
        }
    }
    auto end_lookup_cache = std::chrono::high_resolution_clock::now();
    auto lookup_cache_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_lookup_cache - start_lookup_cache).count();
    std::cout << "[ STRESS TEST ] Finished cached lookups in " << lookup_cache_duration << " ms." << std::endl;
     if (NUM_LOOKUPS > 0) {
        std::cout << "[ STRESS TEST ] Average time per lookup (with cache): " << static_cast<double>(lookup_cache_duration * 1000) / NUM_LOOKUPS << " us." << std::endl;
    }

    if (lookup_cache_duration > 0) {
        double speedup = static_cast<double>(lookup_nocache_duration) / lookup_cache_duration;
        std::cout << "[ STRESS TEST ] Cache speedup: " << speedup << "x" << std::endl;
    }
    
    SUCCEED() << "Stress test completed. Check console output for performance metrics.";
} 