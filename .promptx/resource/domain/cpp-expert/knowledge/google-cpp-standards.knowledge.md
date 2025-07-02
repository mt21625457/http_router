# Google C++ Style Guide 标准知识

## 命名约定深度理解

### 类名规范 (PascalCase)
```cpp
// ✅ 正确示例
class HttpRouter
class RouteHandler
class ConnectionPool
class MessageQueue

// ❌ 错误示例
class httpRouter      // 应该首字母大写
class route_handler   // 不应该使用下划线
class connectionpool  // 应该有驼峰分隔
```

### 函数名规范 (snake_case)
```cpp
// ✅ 正确示例
void ProcessRequest();
bool IsValidRoute(const std::string& path);
int GetRouteCount() const;
void SetMaxConnections(int count);

// ❌ 错误示例
void processRequest();    // 应该使用下划线
bool isValidRoute();      // 同上
int getRouteCount();      // 同上
void SetMaxConnections(); // 驼峰命名应该改为下划线
```

### 变量名规范 (snake_case)
```cpp
// ✅ 正确示例
int route_count = 0;
std::string base_path = "/api";
bool is_running = false;
const int MAX_CONNECTIONS = 1000;

// ❌ 错误示例
int routeCount = 0;       // 应该使用下划线
std::string basePath;     // 同上
bool isRunning = false;   // 同上
const int maxConnections; // 常量应该全大写
```

### 成员变量规范 (snake_case + 后缀下划线)
```cpp
class HttpServer {
 private:
  // ✅ 正确示例
  std::string server_name_;
  int port_number_;
  bool is_secure_;
  std::vector<Route> routes_;
  
  // ❌ 错误示例
  std::string serverName;   // 缺少下划线后缀
  int portNumber_;          // 应该使用snake_case
  bool isSecure;            // 缺少下划线后缀
  std::vector<Route> routes;// 缺少下划线后缀
};
```

## 代码格式化标准

### 缩进和空格规范
```cpp
// ✅ 正确的缩进 (2个空格)
class MyClass {
 public:
  void DoSomething() {
    if (condition) {
      ProcessData();
    }
  }
  
 private:
  int value_;
};

// ❌ 错误的缩进
class MyClass {
    public:  // 4个空格，应该是2个
  void DoSomething() {  // 不一致的缩进
        if (condition) {  // 过多缩进
            ProcessData();
        }
    }
    
    private:
    int value_;
};
```

### 行长度控制 (80字符)
```cpp
// ✅ 正确的行长度控制
bool IsValidHttpRequest(const std::string& method,
                       const std::string& path,
                       const HeaderMap& headers);

// ❌ 超过80字符的行
bool IsValidHttpRequest(const std::string& method, const std::string& path, const HeaderMap& headers, const std::string& body);
```

### 大括号风格 (K&R)
```cpp
// ✅ 正确的K&R风格
if (condition) {
  DoSomething();
} else {
  DoSomethingElse();
}

for (const auto& item : items) {
  ProcessItem(item);
}

class MyClass {
 public:
  void Method() {
    // implementation
  }
};

// ❌ 错误的大括号风格
if (condition)
{  // 大括号应该不换行
  DoSomething();
}
else
{
  DoSomethingElse();
}
```

## 头文件组织规范

### 包含文件顺序标准
```cpp
// my_http_server.cc
#include "my_http_server.h"        // 1. 对应头文件

#include <sys/socket.h>            // 2. C系统头文件
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>               // 3. C++标准库头文件
#include <memory>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>  // 4. 第三方库头文件
#include <gtest/gtest.h>

#include "base/logging.h"          // 5. 项目内头文件
#include "util/string_util.h"
```

### 头文件保护和前置声明
```cpp
// my_class.h
#pragma once  // 推荐使用

#include <memory>
#include <string>

// 前置声明优于包含头文件
class HttpRequest;
class HttpResponse;

class HttpHandler {
 public:
  explicit HttpHandler(const std::string& name);
  
  // 使用前置声明的类型
  void HandleRequest(const HttpRequest& request, HttpResponse* response);
  
 private:
  std::string name_;
  std::unique_ptr<HttpProcessor> processor_;  // 需要完整定义
};
```

## 现代C++特性应用标准

### 智能指针使用规范
```cpp
// ✅ 正确的智能指针使用
class ResourceManager {
 public:
  // 独占所有权使用unique_ptr
  std::unique_ptr<Connection> CreateConnection();
  
  // 共享所有权使用shared_ptr
  std::shared_ptr<CacheEntry> GetCacheEntry(const std::string& key);
  
  // 避免循环引用使用weak_ptr
  void SetParent(std::weak_ptr<ResourceManager> parent);
  
 private:
  std::vector<std::unique_ptr<Resource>> resources_;
  std::shared_ptr<ConfigManager> config_;
};

// ❌ 错误的指针使用
class ResourceManager {
 public:
  Connection* CreateConnection();  // 应该使用智能指针
  void DeleteConnection(Connection* conn);  // 手动内存管理
  
 private:
  std::vector<Resource*> resources_;  // 裸指针容器风险高
};
```

### auto类型推导最佳实践
```cpp
// ✅ 适当使用auto
auto it = container.begin();  // 迭代器类型复杂
auto lambda = [](int x) { return x * 2; };  // lambda类型复杂
auto ptr = std::make_unique<HttpHandler>("handler");  // 智能指针类型明确

// ✅ 不使用auto的情况
int count = 0;           // 简单类型保持明确
bool is_valid = true;    // 布尔值保持明确
double ratio = 0.5;      // 数值类型保持明确

// ❌ 不当使用auto
auto x = 0;              // 不明确是int还是其他整数类型
auto result = SomeFunction();  // 返回类型不明确
```

### constexpr和const使用
```cpp
// ✅ 正确的constexpr使用
constexpr int kMaxConnections = 1000;
constexpr double kTimeoutSeconds = 30.0;

constexpr int CalculateBufferSize(int connections) {
  return connections * 1024;
}

// ✅ 正确的const使用
class HttpServer {
 public:
  int GetPort() const;  // 不修改对象状态的方法
  void SetPort(int port);  // 修改对象状态的方法不加const
  
  // const引用传递参数
  void ProcessRequest(const HttpRequest& request);
  
 private:
  const int default_port_ = 8080;  // 不可修改的成员
  mutable std::mutex mutex_;       // 可在const方法中修改
};
```

## 错误处理和异常安全

### 异常安全保证
```cpp
// ✅ 异常安全的资源管理
class ConnectionPool {
 public:
  void AddConnection(std::unique_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);  // RAII锁管理
    connections_.push_back(std::move(conn));   // 异常安全的移动
  }
  
  std::unique_ptr<Connection> GetConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connections_.empty()) {
      throw std::runtime_error("No available connections");
    }
    auto conn = std::move(connections_.back());
    connections_.pop_back();
    return conn;  // 异常安全的返回
  }
  
 private:
  std::vector<std::unique_ptr<Connection>> connections_;
  std::mutex mutex_;
};
```

### RAII原则应用
```cpp
// ✅ RAII资源管理
class FileProcessor {
 public:
  explicit FileProcessor(const std::string& filename) 
      : file_(fopen(filename.c_str(), "r")) {
    if (!file_) {
      throw std::runtime_error("Failed to open file: " + filename);
    }
  }
  
  ~FileProcessor() {
    if (file_) {
      fclose(file_);  // 自动清理资源
    }
  }
  
  // 禁止复制，允许移动
  FileProcessor(const FileProcessor&) = delete;
  FileProcessor& operator=(const FileProcessor&) = delete;
  
  FileProcessor(FileProcessor&& other) noexcept : file_(other.file_) {
    other.file_ = nullptr;
  }
  
 private:
  FILE* file_;
};
```

## 性能优化指导原则

### 移动语义优化
```cpp
// ✅ 正确使用移动语义
class MessageQueue {
 public:
  void PushMessage(Message message) {  // 按值传递，允许移动
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push_back(std::move(message));  // 移动到容器
  }
  
  Message PopMessage() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (messages_.empty()) {
      return {};  // 返回默认构造的对象
    }
    Message msg = std::move(messages_.front());  // 移动构造
    messages_.pop_front();
    return msg;  // 返回值优化(RVO)
  }
  
 private:
  std::deque<Message> messages_;
  std::mutex mutex_;
};
```

### 容器选择和优化
```cpp
// ✅ 根据使用模式选择合适的容器
class HttpRouter {
 private:
  // 频繁查找用unordered_map
  std::unordered_map<std::string, Handler> route_handlers_;
  
  // 有序遍历用map
  std::map<int, std::string> status_codes_;
  
  // 频繁插入删除用deque
  std::deque<PendingRequest> pending_requests_;
  
  // 固定大小用array
  std::array<char, 4096> buffer_;
  
  // 预分配vector避免重新分配
  std::vector<Connection> connections_;
  
 public:
  HttpRouter() {
    connections_.reserve(1000);  // 预分配空间
  }
};
```

## 线程安全编程规范

### 同步原语使用
```cpp
// ✅ 正确的线程安全设计  
class ThreadSafeCounter {
 public:
  void Increment() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++count_;
  }
  
  int GetValue() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
  }
  
  // 原子操作版本（更高效）
  void AtomicIncrement() {
    atomic_count_.fetch_add(1, std::memory_order_relaxed);
  }
  
  int GetAtomicValue() const {
    return atomic_count_.load(std::memory_order_relaxed);
  }
  
 private:
  mutable std::mutex mutex_;
  int count_ = 0;
  std::atomic<int> atomic_count_{0};
};
``` 