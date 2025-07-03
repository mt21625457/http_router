/**
 * @file massive_lambda_performance_test.cpp
 * @brief 大规模Lambda路由性能测试 - Massive Lambda Route Performance Test
 * @author AI Assistant
 * @date 2025-01-20
 * 
 * 本测试创建超过8000个Lambda表达式路由，并在10个线程中进行并发性能测试
 * This test creates over 8000 Lambda expression routes and performs concurrent performance testing across 10 threads
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <functional>

#include "router/router.hpp"

using namespace flc;
using namespace std::chrono;

/**
 * @brief 大规模Lambda性能测试类
 */
class MassiveLambdaPerformanceTest : public ::testing::Test 
{
protected:
    static constexpr size_t TOTAL_ROUTES = 8500;  // 超过8000个路由
    static constexpr size_t THREAD_COUNT = 10;    // 10个线程
    static constexpr size_t OPERATIONS_PER_THREAD = 10000; // 每线程操作数
    
    using LambdaHandler = std::function<void()>;
    
    std::unique_ptr<router<LambdaHandler>> router_;
    std::vector<std::string> test_paths_;
    std::vector<HttpMethod> test_methods_;
    
    // 性能统计
    struct PerformanceStats {
        std::atomic<uint64_t> total_operations{0};
        std::atomic<uint64_t> successful_operations{0};
        std::atomic<uint64_t> failed_operations{0};
        std::atomic<uint64_t> total_time_ns{0};
        std::atomic<uint64_t> cache_hits{0};
        std::atomic<uint64_t> parameter_extractions{0};
    };
    
    PerformanceStats stats_;
    
    void SetUp() override 
    {
        router_ = std::make_unique<router<LambdaHandler>>();
        
        std::cout << "\n=== 开始创建 " << TOTAL_ROUTES << " 个Lambda路由 ===\n";
        auto start_setup = high_resolution_clock::now();
        
        CreateMassiveLambdaRoutes();
        
        auto end_setup = high_resolution_clock::now();
        auto setup_time = duration_cast<milliseconds>(end_setup - start_setup);
        
        std::cout << "路由创建完成，耗时: " << setup_time.count() << " ms\n";
        std::cout << "平均每个路由创建时间: " 
                  << (setup_time.count() * 1000.0 / TOTAL_ROUTES) << " μs\n";
        std::cout << "=== 路由创建完成 ===\n\n";
    }
    
    void TearDown() override 
    {
        router_.reset();
    }

private:
    /**
     * @brief 创建大规模Lambda路由
     */
    void CreateMassiveLambdaRoutes()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        size_t route_count = 0;
        
        // 1. 静态路由 (30% - 约2550个)
        CreateStaticLambdaRoutes(route_count, gen);
        
        // 2. 参数化路由 (50% - 约4250个)
        CreateParameterizedLambdaRoutes(route_count, gen);
        
        // 3. 通配符路由 (20% - 约1700个)
        CreateWildcardLambdaRoutes(route_count, gen);
        
        std::cout << "总计创建路由数: " << route_count << "\n";
    }
    
    /**
     * @brief 创建静态Lambda路由
     */
    void CreateStaticLambdaRoutes(size_t& route_count, std::mt19937& gen)
    {
        std::uniform_int_distribution<> method_dist(0, 6);
        std::vector<HttpMethod> methods = {
            HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT, 
            HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD, HttpMethod::OPTIONS
        };
        
        std::vector<std::string> categories = {
            "api", "admin", "user", "product", "order", "payment", "shipping",
            "inventory", "analytics", "reports", "settings", "config", "auth",
            "notifications", "messages", "files", "uploads", "downloads"
        };
        
        std::vector<std::string> resources = {
            "dashboard", "profile", "settings", "list", "create", "update", 
            "delete", "view", "edit", "search", "filter", "export", "import",
            "backup", "restore", "sync", "validate", "process", "queue"
        };
        
        for (size_t i = 0; i < TOTAL_ROUTES * 0.3 && route_count < TOTAL_ROUTES; ++i) {
            HttpMethod method = methods[method_dist(gen)];
            std::string category = categories[gen() % categories.size()];
            std::string resource = resources[gen() % resources.size()];
            
            std::string path = "/" + category + "/" + resource + "/" + std::to_string(i);
            
            // 创建唯一的Lambda表达式
            auto lambda = [route_count, path, method]() {
                // 模拟业务逻辑
                thread_local static std::atomic<int> call_count{0};
                call_count.fetch_add(1, std::memory_order_relaxed);
                
                // 模拟一些计算
                volatile int result = 0;
                for (int j = 0; j < 10; ++j) {
                    result += j * call_count.load();
                }
            };
            
            router_->add_route(method, path, std::move(lambda));
            test_paths_.push_back(path);
            test_methods_.push_back(method);
            ++route_count;
        }
        
        std::cout << "静态路由创建完成: " << (route_count) << "\n";
    }
    
    /**
     * @brief 创建参数化Lambda路由
     */
    void CreateParameterizedLambdaRoutes(size_t& route_count, std::mt19937& gen)
    {
        std::uniform_int_distribution<> method_dist(0, 6);
        std::vector<HttpMethod> methods = {
            HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT, 
            HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD, HttpMethod::OPTIONS
        };
        
        std::vector<std::pair<std::string, std::string>> patterns = {
            {"/api/users/:id", "/api/users/123"},
            {"/api/users/:id/posts/:post_id", "/api/users/123/posts/456"},
            {"/api/companies/:company_id/employees/:employee_id", "/api/companies/789/employees/101"},
            {"/api/projects/:project_id/tasks/:task_id/comments/:comment_id", "/api/projects/111/tasks/222/comments/333"},
            {"/api/orders/:order_id/items/:item_id", "/api/orders/444/items/555"},
            {"/api/customers/:customer_id/addresses/:address_id", "/api/customers/666/addresses/777"},
            {"/api/products/:product_id/variants/:variant_id", "/api/products/888/variants/999"},
            {"/api/categories/:category_id/subcategories/:subcategory_id", "/api/categories/111/subcategories/222"},
            {"/api/campaigns/:campaign_id/metrics/:metric_id", "/api/campaigns/333/metrics/444"},
            {"/api/reports/:report_id/sections/:section_id", "/api/reports/555/sections/666"}
        };
        
        size_t start_count = route_count;
        for (size_t i = 0; i < TOTAL_ROUTES * 0.5 && route_count < TOTAL_ROUTES; ++i) {
            HttpMethod method = methods[method_dist(gen)];
            auto pattern_pair = patterns[gen() % patterns.size()];
            std::string pattern = pattern_pair.first + "/action" + std::to_string(i);
            std::string test_path = pattern_pair.second + "/action" + std::to_string(i);
            
            // 创建参数化Lambda表达式
            auto lambda = [route_count, pattern, method, i]() {
                // 模拟参数处理逻辑
                thread_local static std::unordered_map<std::string, int> param_cache;
                std::string key = "param_" + std::to_string(i);
                param_cache[key] = param_cache[key] + 1;
                
                // 模拟数据库查询
                volatile int db_result = 0;
                for (int j = 0; j < 20; ++j) {
                    db_result += j * param_cache[key];
                }
            };
            
            router_->add_route(method, pattern, std::move(lambda));
            test_paths_.push_back(test_path);
            test_methods_.push_back(method);
            ++route_count;
        }
        
        std::cout << "参数化路由创建完成: " << (route_count - start_count) << "\n";
    }
    
    /**
     * @brief 创建通配符Lambda路由
     */
    void CreateWildcardLambdaRoutes(size_t& route_count, std::mt19937& gen)
    {
        std::uniform_int_distribution<> method_dist(0, 6);
        std::vector<HttpMethod> methods = {
            HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT, 
            HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD, HttpMethod::OPTIONS
        };
        
        std::vector<std::string> wildcard_bases = {
            "/static/assets", "/uploads/files", "/downloads/docs", 
            "/media/images", "/cache/data", "/temp/storage",
            "/backups/archive", "/logs/system", "/config/templates"
        };
        
        size_t start_count = route_count;
        for (size_t i = 0; i < TOTAL_ROUTES * 0.2 && route_count < TOTAL_ROUTES; ++i) {
            HttpMethod method = methods[method_dist(gen)];
            std::string base = wildcard_bases[gen() % wildcard_bases.size()];
            std::string pattern = base + std::to_string(i % 1000) + "/*";
            
            // 创建通配符Lambda表达式
            auto lambda = [route_count, pattern, method, i]() {
                // 模拟文件处理逻辑
                thread_local static std::atomic<size_t> file_count{0};
                file_count.fetch_add(1, std::memory_order_relaxed);
                
                // 模拟文件操作
                volatile size_t file_size = 0;
                for (size_t j = 0; j < 15; ++j) {
                    file_size += j + file_count.load();
                }
            };
            
            router_->add_route(method, pattern, std::move(lambda));
            
            // 为测试创建对应的实际路径
            std::string test_path = base + std::to_string(i % 1000) + "/file" + std::to_string(i) + ".dat";
            test_paths_.push_back(test_path);
            test_methods_.push_back(method);
            ++route_count;
        }
        
        std::cout << "通配符路由创建完成: " << (route_count - start_count) << "\n";
    }

public:
    /**
     * @brief 多线程性能测试
     */
    void RunMultiThreadPerformanceTest()
    {
        std::cout << "\n=== 开始多线程性能测试 ===\n";
        std::cout << "线程数: " << THREAD_COUNT << "\n";
        std::cout << "每线程操作数: " << OPERATIONS_PER_THREAD << "\n";
        std::cout << "总操作数: " << (THREAD_COUNT * OPERATIONS_PER_THREAD) << "\n\n";
        
        std::vector<std::thread> threads;
        std::atomic<bool> start_flag{false};
        
        auto thread_worker = [this, &start_flag](int thread_id) {
            // 等待开始信号
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::random_device rd;
            std::mt19937 gen(rd() + thread_id);
            std::uniform_int_distribution<> path_dist(0, test_paths_.size() - 1);
            
            auto thread_start = high_resolution_clock::now();
            uint64_t local_successful = 0;
            uint64_t local_failed = 0;
            uint64_t local_cache_hits = 0;
            uint64_t local_param_extractions = 0;
            
            for (size_t i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                auto op_start = high_resolution_clock::now();
                
                // 随机选择测试路径和方法
                size_t path_idx = path_dist(gen);
                std::string& test_path = test_paths_[path_idx];
                HttpMethod test_method = test_methods_[path_idx];
                
                std::map<std::string, std::string> params, query_params;
                auto result = router_->find_route(test_method, test_path, params, query_params);
                
                auto op_end = high_resolution_clock::now();
                auto op_time = duration_cast<nanoseconds>(op_end - op_start);
                
                if (result.has_value()) {
                    // 执行Lambda
                    result.value().get()();
                    local_successful++;
                    
                    // 统计缓存命中（简化检测）
                    if (op_time.count() < 1000) { // 小于1μs认为是缓存命中
                        local_cache_hits++;
                    }
                    
                    // 统计参数提取
                    if (!params.empty()) {
                        local_param_extractions++;
                    }
                } else {
                    local_failed++;
                }
                
                stats_.total_time_ns.fetch_add(op_time.count(), std::memory_order_relaxed);
            }
            
            auto thread_end = high_resolution_clock::now();
            auto thread_time = duration_cast<milliseconds>(thread_end - thread_start);
            
            // 更新全局统计
            stats_.successful_operations.fetch_add(local_successful, std::memory_order_relaxed);
            stats_.failed_operations.fetch_add(local_failed, std::memory_order_relaxed);
            stats_.cache_hits.fetch_add(local_cache_hits, std::memory_order_relaxed);
            stats_.parameter_extractions.fetch_add(local_param_extractions, std::memory_order_relaxed);
            
            std::cout << "线程 " << thread_id << " 完成: " 
                      << local_successful << "/" << OPERATIONS_PER_THREAD 
                      << " 成功, 耗时: " << thread_time.count() << " ms, "
                      << "平均: " << (thread_time.count() * 1000.0 / OPERATIONS_PER_THREAD) << " μs/op\n";
        };
        
        // 创建并启动线程
        for (int i = 0; i < THREAD_COUNT; ++i) {
            threads.emplace_back(thread_worker, i);
        }
        
        // 等待短暂时间确保所有线程准备就绪
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        auto total_start = high_resolution_clock::now();
        
        // 发出开始信号
        start_flag.store(true);
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto total_end = high_resolution_clock::now();
        auto total_time = duration_cast<milliseconds>(total_end - total_start);
        
        // 计算和输出统计结果
        PrintPerformanceResults(total_time);
    }

private:
    /**
     * @brief 输出性能测试结果
     */
    void PrintPerformanceResults(const std::chrono::milliseconds& total_time)
    {
        uint64_t total_ops = stats_.successful_operations.load() + stats_.failed_operations.load();
        double success_rate = total_ops > 0 ? (double)stats_.successful_operations.load() / total_ops * 100.0 : 0.0;
        double cache_hit_rate = total_ops > 0 ? (double)stats_.cache_hits.load() / total_ops * 100.0 : 0.0;
        double avg_time_ns = total_ops > 0 ? (double)stats_.total_time_ns.load() / total_ops : 0.0;
        
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "           大规模Lambda路由性能测试结果\n";
        std::cout << std::string(60, '=') << "\n";
        std::cout << "总路由数:           " << TOTAL_ROUTES << "\n";
        std::cout << "线程数:             " << THREAD_COUNT << "\n";
        std::cout << "总操作数:           " << total_ops << "\n";
        std::cout << "成功操作:           " << stats_.successful_operations.load() << "\n";
        std::cout << "失败操作:           " << stats_.failed_operations.load() << "\n";
        std::cout << "成功率:             " << std::fixed << std::setprecision(2) << success_rate << "%\n";
        std::cout << "缓存命中:           " << stats_.cache_hits.load() << "\n";
        std::cout << "缓存命中率:         " << std::fixed << std::setprecision(2) << cache_hit_rate << "%\n";
        std::cout << "参数提取次数:       " << stats_.parameter_extractions.load() << "\n";
        std::cout << "\n";
        std::cout << "总耗时:             " << total_time.count() << " ms\n";
        std::cout << "平均每操作时间:     " << std::fixed << std::setprecision(3) << avg_time_ns / 1000.0 << " μs\n";
        std::cout << "吞吐量:             " << std::fixed << std::setprecision(0) 
                  << (total_ops * 1000.0 / total_time.count()) << " ops/sec\n";
        std::cout << "每线程吞吐量:       " << std::fixed << std::setprecision(0) 
                  << (total_ops * 1000.0 / total_time.count() / THREAD_COUNT) << " ops/sec\n";
        std::cout << std::string(60, '=') << "\n\n";
    }
};

/**
 * @brief 大规模Lambda路由并发性能测试
 */
TEST_F(MassiveLambdaPerformanceTest, ConcurrentLambdaRoutePerformance)
{
    // 验证路由数量
    EXPECT_GE(test_paths_.size(), 8000) << "路由数量应该超过8000个";
    EXPECT_EQ(test_paths_.size(), test_methods_.size()) << "路径和方法数量应该一致";
    
    // 执行多线程性能测试
    RunMultiThreadPerformanceTest();
    
    // 验证测试结果
    uint64_t total_ops = stats_.successful_operations.load() + stats_.failed_operations.load();
    uint64_t expected_ops = THREAD_COUNT * OPERATIONS_PER_THREAD;
    
    EXPECT_EQ(total_ops, expected_ops) << "总操作数应该等于预期数量";
    EXPECT_GT(stats_.successful_operations.load(), expected_ops * 0.50) << "成功率应该大于50%";
    EXPECT_LT(stats_.failed_operations.load(), expected_ops * 0.50) << "失败率应该小于50%";
    
    // 性能要求
    double avg_time_us = stats_.total_time_ns.load() / 1000.0 / total_ops;
    EXPECT_LT(avg_time_us, 100.0) << "平均每操作时间应该小于100μs";
    
    std::cout << "✅ 大规模Lambda路由并发性能测试完成!\n";
    std::cout << "📊 测试了 " << test_paths_.size() << " 个Lambda路由\n";
    std::cout << "🚀 在 " << THREAD_COUNT << " 个线程中执行了 " << total_ops << " 次操作\n";
    std::cout << "⚡ 平均响应时间: " << std::fixed << std::setprecision(3) << avg_time_us << " μs\n";
}

/**
 * @brief 内存压力测试
 */
TEST_F(MassiveLambdaPerformanceTest, MemoryStressTest)
{
    std::cout << "\n=== 内存压力测试 ===\n";
    
    // 创建大量并发操作
    std::vector<std::thread> stress_threads;
    std::atomic<bool> stop_flag{false};
    std::atomic<uint64_t> stress_operations{0};
    
    auto stress_worker = [this, &stop_flag, &stress_operations]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> path_dist(0, test_paths_.size() - 1);
        
        while (!stop_flag.load()) {
            size_t path_idx = path_dist(gen);
            std::map<std::string, std::string> params, query_params;
            
            auto result = router_->find_route(test_methods_[path_idx], test_paths_[path_idx], params, query_params);
            if (result.has_value()) {
                result.value().get()();
            }
            
            stress_operations.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    // 启动压力测试线程
    for (int i = 0; i < THREAD_COUNT * 2; ++i) {
        stress_threads.emplace_back(stress_worker);
    }
    
    // 运行5秒钟
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    stop_flag.store(true);
    for (auto& thread : stress_threads) {
        thread.join();
    }
    
    std::cout << "内存压力测试完成，总操作数: " << stress_operations.load() << "\n";
    std::cout << "平均每秒操作数: " << (stress_operations.load() / 5) << "\n";
    
    EXPECT_GT(stress_operations.load(), 100000) << "5秒内应该完成至少10万次操作";
}

/**
 * @brief Lambda表达式多样性测试
 */
TEST_F(MassiveLambdaPerformanceTest, LambdaDiversityTest)
{
    std::cout << "\n=== Lambda表达式多样性测试 ===\n";
    
    auto diversity_router = std::make_unique<router<LambdaHandler>>();
    
    // 不同类型的Lambda表达式
    std::vector<std::function<LambdaHandler()>> lambda_factories = {
        // 1. 简单Lambda
        []() { return []() { volatile int x = 42; }; },
        
        // 2. 捕获Lambda
        [&]() { return [this]() { volatile auto ptr = router_.get(); }; },
        
        // 3. 有状态Lambda
        []() { return []() mutable { static int counter = 0; ++counter; }; },
        
        // 4. 复杂计算Lambda
        []() { return []() {
            volatile double result = 0.0;
            for (int i = 0; i < 100; ++i) {
                result += std::sin(i) * std::cos(i);
            }
        }; },
        
        // 5. 容器操作Lambda
        []() { return []() {
            thread_local std::vector<int> data(100);
            std::iota(data.begin(), data.end(), 0);
            std::sort(data.begin(), data.end());
        }; }
    };
    
    // 为每种类型创建多个路由
    size_t route_count = 0;
    for (size_t type = 0; type < lambda_factories.size(); ++type) {
        for (size_t i = 0; i < 200; ++i) {
            std::string path = "/lambda/" + std::to_string(type) + "/" + std::to_string(i);
            auto lambda = lambda_factories[type]();
            diversity_router->add_route(HttpMethod::GET, path, std::move(lambda));
            ++route_count;
        }
    }
    
    std::cout << "创建了 " << route_count << " 个不同类型的Lambda路由\n";
    
    // 测试执行
    auto start = high_resolution_clock::now();
    for (size_t type = 0; type < lambda_factories.size(); ++type) {
        for (size_t i = 0; i < 200; ++i) {
            std::string path = "/lambda/" + std::to_string(type) + "/" + std::to_string(i);
            std::map<std::string, std::string> params, query_params;
            auto result = diversity_router->find_route(HttpMethod::GET, path, params, query_params);
            ASSERT_TRUE(result.has_value());
            result.value().get()();
        }
    }
    auto end = high_resolution_clock::now();
    
    auto total_time = duration_cast<microseconds>(end - start);
    double avg_time = total_time.count() / (double)route_count;
    
    std::cout << "Lambda多样性测试完成\n";
    std::cout << "总时间: " << total_time.count() << " μs\n";
    std::cout << "平均每个Lambda: " << std::fixed << std::setprecision(3) << avg_time << " μs\n";
    
    EXPECT_LT(avg_time, 50.0) << "平均每个Lambda执行时间应该小于50μs";
}