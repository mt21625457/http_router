# HTTP Router

高性能C++路由库，具有高效的路径匹配和缓存机制。

## 项目介绍

HTTP Router 是一个仅头文件的C++库，专为快速URL路径匹配而设计，支持：

- 静态路由（`/about`，`/products`）
- 参数化路由（`/users/:id`，`/api/:version/resources/:resourceId`）
- 通配符路由（`/static/*`，`/files/:type/*`）
- 查询参数解析（`?key=value&flag=true`）
- 优化的路由缓存，提高性能
- Lambda表达式支持，用于现代C++开发
- 线程安全的并发访问，支持SIMD优化

该库实现了智能混合匹配算法，结合了哈希表、前缀树和LRU缓存的优势，为不同的路由模式实现最佳性能。

## 技术要求

- C++17或更高版本（推荐C++20）
- 仅头文件（无需编译）
- 除`tsl::htrie_map`（用于前缀树操作）外无外部依赖
- CMake 3.10+用于构建测试
- GoogleTest用于运行测试套件

## 主要特性

- **高性能**：针对快速路由匹配进行优化，支持SIMD优化
- **灵活路由**：支持静态、参数化和通配符路由
- **查询参数解析**：内置对URL查询参数的支持
- **路由缓存**：实现LRU缓存，优化常用路由访问
- **仅头文件**：易于集成，无需编译
- **模板设计**：适用于任何处理程序类型
- **内存效率**：根据路由特性使用适当的数据结构
- **Lambda支持**：完整支持C++ Lambda表达式作为路由处理器
- **线程安全**：支持并发访问，使用原子操作和无锁结构
- **对象池**：通过预分配对象池进行内存优化
- **快速路径缓存**：为常用路由提供专门的缓存
- **HTTP方法支持**：完整支持所有标准HTTP方法

## 使用示例

### 基本用法

```cpp
#include "router/router.hpp"
#include <iostream>
#include <functional>

using namespace flc;

// 定义一个简单的处理程序类型
using RouteHandler = std::function<void(const std::map<std::string, std::string>&)>;

int main() {
    // 创建路由器实例
    router<RouteHandler> router_;
    
    // 添加静态路由
    router_.add_route(HttpMethod::GET, "/home", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "首页" << std::endl;
        }
    );
    
    // 添加参数化路由
    router_.add_route(HttpMethod::GET, "/users/:id", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "用户: " << params.at("id") << std::endl;
        }
    );
    
    // 添加通配符路由
    router_.add_route(HttpMethod::GET, "/files/*", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "文件路径: " << params.at("*") << std::endl;
        }
    );
    
    // 处理请求
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // 匹配静态路由
    auto result = router_.find_route(HttpMethod::GET, "/home", params, query_params);
    if (result.has_value()) {
        result.value().get()(params);  // 输出: 首页
    }
    
    // 匹配参数化路由
    result = router_.find_route(HttpMethod::GET, "/users/123", params, query_params);
    if (result.has_value()) {
        result.value().get()(params);  // 输出: 用户: 123
    }
    
    // 匹配带查询参数的通配符路由
    result = router_.find_route(HttpMethod::GET, "/files/documents/report.pdf?version=latest", params, query_params);
    if (result.has_value()) {
        result.value().get()(params);  // 输出: 文件路径: documents/report.pdf
        std::cout << "版本: " << query_params["version"] << std::endl;  // 输出: 版本: latest
    }
    
    return 0;
}
```

### 自定义处理程序类

```cpp
#include "router/router.hpp"
#include <iostream>

using namespace flc;

// 自定义处理程序类
class RequestHandler {
public:
    RequestHandler(const std::string& name) : name_(name) {}
    
    void operator()(const std::map<std::string, std::string>& params) {
        std::cout << "处理程序 '" << name_ << "' 正在处理请求" << std::endl;
        
        for (const auto& [key, value] : params) {
            std::cout << "  参数: " << key << " = " << value << std::endl;
        }
    }
    
    void handle(const std::map<std::string, std::string>& params,
                const std::map<std::string, std::string>& query_params) {
        std::cout << "处理程序 '" << name_ << "' 正在处理请求" << std::endl;
        
        for (const auto& [key, value] : params) {
            std::cout << "  参数: " << key << " = " << value << std::endl;
        }
        
        for (const auto& [key, value] : query_params) {
            std::cout << "  查询: " << key << " = " << value << std::endl;
        }
    }
    
private:
    std::string name_;
};

int main() {
    router<RequestHandler> router_;
    
    // 注册路由
    router_.add_route(HttpMethod::GET, "/api/users", RequestHandler("列出用户"));
    router_.add_route(HttpMethod::GET, "/api/users/:id", RequestHandler("获取用户"));
    router_.add_route(HttpMethod::GET, "/api/users/:id/posts", RequestHandler("获取用户文章"));
    
    // 匹配路由
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // 处理请求
    auto result = router_.find_route(HttpMethod::GET, "/api/users/42?fields=name,email", params, query_params);
    if (result.has_value()) {
        result.value().get().handle(params, query_params);
    }
    
    return 0;
}
```

### Lambda表达式支持

```cpp
#include "router/router.hpp"
#include <iostream>

using namespace flc;

int main() {
    // 使用Lambda处理器的路由器
    router<std::function<void()>> router_;
    
    // 简单Lambda处理器
    router_.add_route(HttpMethod::GET, "/hello", 
        []() {
            std::cout << "你好，世界！" << std::endl;
        });
    
    // 需要参数的处理器路由器
    router<std::function<void(const std::map<std::string, std::string>&)>> param_router;
    
    // 带参数提取的Lambda
    param_router.add_route(HttpMethod::GET, "/users/:id", 
        [](const std::map<std::string, std::string>& params) {
            std::cout << "用户ID: " << params.at("id") << std::endl;
        });
    
    // 带捕获的Lambda
    std::string server_name = "我的服务器";
    router_.add_route(HttpMethod::GET, "/status", 
        [server_name]() {
            std::cout << "服务器: " << server_name << " 正在运行" << std::endl;
        });
    
    // 测试路由
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // 测试简单Lambda路由
    auto result = router_.find_route(HttpMethod::GET, "/hello", params, query_params);
    if (result.has_value()) {
        result.value().get()();
    }
    
    // 测试参数化Lambda路由
    auto param_result = param_router.find_route(HttpMethod::GET, "/users/123", params, query_params);
    if (param_result.has_value()) {
        param_result.value().get()(params);
    }
    
    return 0;
}
```

### HTTP方法支持

```cpp
#include "router/router.hpp"
#include <iostream>

using namespace flc;

int main() {
    router<std::function<void()>> router_;
    
    // 添加不同HTTP方法的路由
    router_.add_route(HttpMethod::GET, "/api/users", 
        []() { std::cout << "GET: 获取用户列表" << std::endl; });
    
    router_.add_route(HttpMethod::POST, "/api/users", 
        []() { std::cout << "POST: 创建用户" << std::endl; });
    
    router_.add_route(HttpMethod::PUT, "/api/users/:id", 
        []() { std::cout << "PUT: 更新用户" << std::endl; });
    
    router_.add_route(HttpMethod::DELETE, "/api/users/:id", 
        []() { std::cout << "DELETE: 删除用户" << std::endl; });
    
    // 按方法查找路由
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    auto result = router_.find_route(HttpMethod::GET, "/api/users", params, query_params);
    if (result.has_value()) {
        result.value().get()(); // 输出: GET: 获取用户列表
    }
    
    result = router_.find_route(HttpMethod::POST, "/api/users", params, query_params);
    if (result.has_value()) {
        result.value().get()(); // 输出: POST: 创建用户
    }
    
    return 0;
}
```

## 测试用例

路由器附带跨12个测试套件的全面测试用例：

### 核心测试套件
1. **`HttpRouterTest`**（15个测试）：基础功能，包括静态、参数化和通配符路由
2. **`HttpRouterPerformanceTest`**（10个测试）：不同路由类型的性能基准测试
3. **`HybridRoutingTest`**（5个测试）：测试混合路由策略（哈希表和前缀树）
4. **`HttpRouterAdvTest`**（4个测试）：高级功能，如URL解码和LRU缓存淘汰
5. **`RouterStressTest`**（4个测试）：大规模路由集和并发访问压力测试

### 优化测试套件
6. **`RouterOptimizationIntegrationTest`**（15个测试）：优化路由的集成测试
7. **`RouterOptimizationTest`**（29个测试）：全面的优化功能测试
8. **`SimpleRouterTest`**（10个测试）：基本路由功能验证
9. **`BasicRouterTest`**（2个测试）：核心路由功能

### 高级测试套件
10. **`LambdaContextTest`**（8个测试）：Lambda表达式支持和性能测试
11. **`MassiveLambdaPerformanceTest`**（3个测试）：大规模Lambda路由性能测试
12. **`MassiveLambdaPerformanceTestFixed`**（1个测试）：大规模Lambda测试的修复版本

### 测试结果摘要
- **总测试数**：跨12个测试套件的106个测试
- **成功率**：100%（106/106测试通过）
- **总测试时间**：约6秒
- **线程安全**：通过16,000个并发操作验证
- **大规模测试**：测试了8,500个路由和100,000个操作

### 运行测试

```bash
# 构建测试套件
mkdir build && cd build
cmake ..
make

# 运行所有测试
./http_router_tests

# 或使用CMake运行测试
cd build
cmake ..
make test
```

## 性能基准

基于包含的测试套件进行的综合测试：

### 路由匹配性能
| 路由类型 | 平均查找时间 | 使用缓存后 | 吞吐量 |
|----------|--------------|------------|--------|
| 静态     | ~0.17 µs     | ~0.03 µs   | ~699K ops/sec |
| 参数化   | ~8.3 µs      | ~0.1 µs    | ~568K ops/sec |
| 通配符   | ~94 µs       | ~0.1 µs    | ~100K ops/sec |

### 优化性能
| 操作 | 时间 | 性能 |
|------|------|------|
| 路径分割 | 0.0326 µs | 高度优化 |
| URL解码 | 0.0511 µs | 安全且快速 |
| 十六进制转换 | 0.00022 ns | 超快速 |
| Lambda处理器 | 0.177 µs | 最小开销 |

### 大规模性能
- **8,500个路由**：成功测试大规模路由集
- **100,000个操作**：处理时缓存命中率达30%+
- **多线程**：10个线程，成功率99.9%+
- **内存效率**：对象池优化内存使用

### 线程安全结果
- **16,000个并发操作**：100%成功率
- **10个线程**：平均每操作13-16微秒
- **缓存命中率**：并发负载下30-31%

*性能测量在现代硬件的Linux系统上进行。结果可能因系统配置而异。*

## 开源许可

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

MIT 许可证

版权所有 (c) 2025 MT

特此免费授予任何获得本软件副本和相关文档文件（"软件"）的人不受限制地处置该软件的权利，
包括不限于使用、复制、修改、合并、发布、分发、再许可和/或出售该软件副本，
以及允许该软件的被提供者这样做，但须符合以下条件：

上述版权声明和本许可声明应包含在该软件的所有副本或实质性部分中。

本软件是"按原样"提供的，没有任何形式的明示或暗示的保证，包括但不限于对适销性、特定用途的适用性和非侵权性的保证。
在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，无论是因合同、侵权或其他方式引起的，
与软件或软件的使用或其他交易有关。 