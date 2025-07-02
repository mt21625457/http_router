<execution>
  <constraint>
    ## Google Style Guide技术约束
    - **行长度限制**：最大80个字符
    - **缩进要求**：使用2个空格，不使用制表符
    - **文件扩展名**：头文件使用`.h`，源文件使用`.cc`
    - **编译兼容性**：必须在指定的C++标准版本下编译
    - **命名空间规则**：避免在头文件中使用`using namespace`
  </constraint>

  <rule>
    ## 强制性风格规则
    - **命名约定强制执行**：
      - 类名使用大驼峰命名法 (PascalCase)
      - 函数名使用小写字母和下划线 (snake_case)
      - 变量名使用小写字母和下划线 (snake_case)
      - 常量名使用全大写字母和下划线
      - 成员变量使用下划线后缀
    - **格式化强制要求**：
      - 所有代码必须通过clang-format格式化
      - 使用K&R风格大括号
      - 条件语句必须使用大括号
    - **包含文件顺序**：严格按照指定顺序包含头文件
  </rule>

  <guideline>
    ## 风格指导原则
    - **可读性优先**：代码应该易于理解和维护
    - **一致性重要**：保持项目内的风格一致
    - **现代C++特性**：优先使用现代C++特性
    - **避免过度设计**：保持代码简洁明了
    - **文档化接口**：公共API必须有清晰的文档
  </guideline>

  <process>
    ## Google Style Guide执行流程

    ### 📝 命名规范检查
    ```mermaid
    flowchart TD
        A[代码审查] --> B{类名检查}
        B -->|不符合| C[修正为PascalCase]
        B -->|符合| D{函数名检查}
        D -->|不符合| E[修正为snake_case]
        D -->|符合| F{变量名检查}
        F -->|不符合| G[修正命名风格]
        F -->|符合| H[检查通过]
        C --> D
        E --> F
        G --> H
    ```

    **命名规范应用**：
    ```cpp
    // ✅ 正确的命名
    class HttpRouter {
     public:
      void AddRoute(const std::string& path);
      bool IsRouteExists(const std::string& path) const;
      
     private:
      std::string base_path_;
      int route_count_;
    };
    
    // ❌ 错误的命名
    class httpRouter {  // 应该是 HttpRouter
     public:
      void addRoute(const std::string& Path);  // 函数名应该是 add_route
      bool isRouteExists(const std::string& path) const;  // 应该是 is_route_exists
      
     private:
      std::string basePath;  // 应该是 base_path_
      int RouteCount;        // 应该是 route_count_
    };
    ```

    ### 🎨 代码格式化流程
    ```mermaid
    flowchart TD
        A[写完代码] --> B[运行clang-format]
        B --> C[检查缩进]
        C --> D[检查行长度]
        D --> E[检查大括号风格]
        E --> F[提交代码]
    ```

    **格式化标准**：
    ```cpp
    // ✅ 正确的格式
    class MyClass {
     public:
      MyClass(const std::string& name, int value);
      
      void DoSomething() {
        if (condition_) {
          ProcessData();
        }
      }
      
     private:
      std::string name_;
      int value_;
    };
    
    // ❌ 错误的格式
    class MyClass
    {  // 大括号应该不换行
        public:  // 缩进应该是2个空格
        MyClass(const std::string& name, int value);
        
        void DoSomething()
        {  // 大括号风格不一致
            if(condition_)  // if后面应该有空格
                ProcessData();  // 应该使用大括号
        }
        
        private:
        std::string name_;
        int value_;
    };
    ```

    ### 📂 头文件包含顺序
    ```mermaid
    flowchart TD
        A[包含头文件] --> B[对应的源文件头文件]
        B --> C[C系统头文件]
        C --> D[C++标准库头文件]
        D --> E[其他库头文件]
        E --> F[项目内头文件]
    ```

    **包含顺序示例**：
    ```cpp
    // my_class.cc
    #include "my_class.h"           // 1. 对应的头文件
    
    #include <sys/types.h>          // 2. C系统头文件
    #include <unistd.h>
    
    #include <algorithm>            // 3. C++标准库头文件
    #include <memory>
    #include <string>
    #include <vector>
    
    #include <gtest/gtest.h>        // 4. 其他库头文件
    
    #include "base/logging.h"       // 5. 项目内头文件
    #include "util/string_util.h"
    ```

    ### 🔍 现代C++特性应用
    ```mermaid
    flowchart TD
        A[代码实现] --> B{内存管理}
        B -->|需要| C[使用智能指针]
        B -->|不需要| D{类型推导}
        D -->|复杂类型| E[使用auto]
        D -->|简单类型| F{空指针}
        F -->|需要| G[使用nullptr]
        F -->|不需要| H[继续检查]
        C --> D
        E --> F
        G --> H
    ```

    **现代C++特性应用**：
    ```cpp
    // ✅ 现代C++风格
    class Router {
     public:
      explicit Router(const std::string& base_path);
      ~Router() = default;
      
      Router(const Router&) = delete;
      Router& operator=(const Router&) = delete;
      
      void AddRoute(std::unique_ptr<Route> route);
      
      template<typename T>
      auto FindRoute(const T& path) const -> std::optional<Route*>;
      
     private:
      std::unique_ptr<RouteTree> routes_;
      std::string base_path_;
    };
    
    // ❌ 旧式C++风格
    class Router {
     public:
      Router(const std::string& base_path);
      ~Router();  // 应该使用 = default
      
      void AddRoute(Route* route);  // 应该使用智能指针
      
      Route* FindRoute(const std::string& path) const;  // 应该使用optional
      
     private:
      RouteTree* routes_;  // 应该使用智能指针
      std::string base_path_;
    };
    ```

    ### 📋 代码审查检查清单
    ```mermaid
    flowchart TD
        A[开始审查] --> B[命名规范]
        B --> C[格式化检查]
        C --> D[头文件顺序]
        D --> E[现代C++特性]
        E --> F[注释质量]
        F --> G[测试覆盖]
        G --> H[审查完成]
    ```
  </process>

  <criteria>
    ## Style Guide遵循标准

    ### 命名合规性
    - ✅ **类名**: PascalCase (如 `HttpRouter`)
    - ✅ **函数名**: snake_case (如 `add_route()`)
    - ✅ **变量名**: snake_case (如 `route_count`)
    - ✅ **常量名**: UPPER_SNAKE_CASE (如 `MAX_ROUTES`)
    - ✅ **成员变量**: snake_case + 下划线后缀 (如 `routes_`)

    ### 格式化标准
    - ✅ **缩进**: 2个空格，无制表符
    - ✅ **行长度**: ≤ 80字符
    - ✅ **大括号**: K&R风格
    - ✅ **空格**: 关键字后、操作符前后有空格
    - ✅ **条件语句**: 即使单行也使用大括号

    ### 代码组织标准
    - ✅ **头文件保护**: 使用 `#pragma once`
    - ✅ **包含顺序**: 按规定顺序包含头文件
    - ✅ **命名空间**: 避免在头文件中使用 `using namespace`
    - ✅ **访问修饰符**: public → protected → private 顺序

    ### 现代C++特性使用
    - ✅ **智能指针**: 优先使用 `std::unique_ptr`, `std::shared_ptr`
    - ✅ **类型推导**: 合理使用 `auto` 关键字
    - ✅ **空指针**: 使用 `nullptr` 而不是 `NULL`
    - ✅ **override**: 虚函数重写使用 `override` 关键字
    - ✅ **默认/删除**: 使用 `= default` 和 `= delete`
  </criteria>
</execution> 