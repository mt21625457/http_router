# HTTP Router

高性能C++路由库，具有高效的路径匹配和缓存机制。

## 项目介绍

HTTP Router 是一个仅头文件的C++库，专为快速URL路径匹配而设计，支持：

- 静态路由（`/about`，`/products`）
- 参数化路由（`/users/:id`，`/api/:version/resources/:resourceId`）
- 通配符路由（`/static/*`，`/files/:type/*`）
- 查询参数解析（`?key=value&flag=true`）
- 优化的路由缓存，提高性能

该库实现了智能混合匹配算法，结合了哈希表、前缀树和LRU缓存的优势，为不同的路由模式实现最佳性能。

## 技术要求

- C++17或更高版本
- 仅头文件（无需编译）
- 除`tsl::htrie_map`（用于前缀树操作）外无外部依赖

## 主要特性

- **高性能**：针对快速路由匹配进行优化
- **灵活路由**：支持静态、参数化和通配符路由
- **查询参数解析**：内置对URL查询参数的支持
- **路由缓存**：实现LRU缓存，优化常用路由访问
- **仅头文件**：易于集成，无需编译
- **模板设计**：适用于任何处理程序类型
- **内存效率**：根据路由特性使用适当的数据结构

## 使用示例

### 基本用法

```cpp
#include "http_router.hpp"
#include <iostream>
#include <functional>

// 定义一个简单的处理程序类型
using RouteHandler = std::function<void(const std::map<std::string, std::string>&)>;

int main() {
    // 创建路由器实例
    http_router<RouteHandler> router;
    
    // 添加静态路由
    router.add_route("/home", std::make_shared<RouteHandler>(
        [](const std::map<std::string, std::string>& params) {
            std::cout << "首页" << std::endl;
        }
    ));
    
    // 添加参数化路由
    router.add_route("/users/:id", std::make_shared<RouteHandler>(
        [](const std::map<std::string, std::string>& params) {
            std::cout << "用户: " << params.at("id") << std::endl;
        }
    ));
    
    // 添加通配符路由
    router.add_route("/files/*", std::make_shared<RouteHandler>(
        [](const std::map<std::string, std::string>& params) {
            std::cout << "文件路径: " << params.at("*") << std::endl;
        }
    ));
    
    // 处理请求
    std::shared_ptr<RouteHandler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // 匹配静态路由
    if (router.find_route("/home", handler, params, query_params) == 0) {
        (*handler)(params);  // 输出: 首页
    }
    
    // 匹配参数化路由
    if (router.find_route("/users/123", handler, params, query_params) == 0) {
        (*handler)(params);  // 输出: 用户: 123
    }
    
    // 匹配带查询参数的通配符路由
    if (router.find_route("/files/documents/report.pdf?version=latest", handler, params, query_params) == 0) {
        (*handler)(params);  // 输出: 文件路径: documents/report.pdf
        std::cout << "版本: " << query_params["version"] << std::endl;  // 输出: 版本: latest
    }
    
    return 0;
}
```

### 自定义处理程序类

```cpp
#include "http_router.hpp"
#include <iostream>

// 自定义处理程序类
class RequestHandler {
public:
    RequestHandler(const std::string& name) : name_(name) {}
    
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
    http_router<RequestHandler> router;
    
    // 注册路由
    router.add_route("/api/users", std::make_shared<RequestHandler>("列出用户"));
    router.add_route("/api/users/:id", std::make_shared<RequestHandler>("获取用户"));
    router.add_route("/api/users/:id/posts", std::make_shared<RequestHandler>("获取用户文章"));
    
    // 匹配路由
    std::shared_ptr<RequestHandler> handler;
    std::map<std::string, std::string> params;
    std::map<std::string, std::string> query_params;
    
    // 处理请求
    if (router.find_route("/api/users/42?fields=name,email", handler, params, query_params) == 0) {
        handler->handle(params, query_params);
    }
    
    return 0;
}
```

## 测试用例

路由器附带全面的测试用例：

1. **`BasicFunctionalityTest`**：测试基本的静态、参数化和通配符路由
2. **`CacheClearTest`**：确保缓存清除正常工作
3. **`CachePerformanceTest`**：测量缓存带来的性能提升
4. **`LRUEvictionTest`**：验证LRU缓存淘汰机制
5. **`RandomAccessPatternTest`**：使用真实世界访问模式测试性能

运行测试的方法：

```bash
mkdir build && cd build
cmake ..
make
./http_router_test
```

## 性能

路由匹配性能因路由类型而异：

| 路由类型 | 平均查找时间 | 使用缓存后 |
|----------|--------------|------------|
| 静态     | ~0.5 µs      | ~0.1 µs    |
| 参数化   | ~2.0 µs      | ~0.1 µs    |
| 通配符   | ~2.5 µs      | ~0.1 µs    |

*时间为近似值，会因硬件和路由复杂度而有所不同。*

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