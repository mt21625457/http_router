# CMakeLists.txt for Router Optimization Tests
# 路由器优化测试的CMake配置文件

cmake_minimum_required(VERSION 3.16)

# 如果这是单独构建测试
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(RouterOptimizationTests CXX)
    
    # 设置C++标准
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
    # 查找依赖
    find_package(GTest REQUIRED)
    find_package(PkgConfig REQUIRED)
    
    # 设置包含目录
    include_directories(../include)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 添加优化集成测试
add_executable(router_optimization_integration_test
    router_optimization_integration_test.cpp
)

target_link_libraries(router_optimization_integration_test
    GTest::gtest
    GTest::gtest_main
    pthread
)

target_include_directories(router_optimization_integration_test PRIVATE
    ../include
)

# 添加优化单元测试
add_executable(router_optimization_test
    router_optimization_test.cpp
)

target_link_libraries(router_optimization_test
    GTest::gtest
    GTest::gtest_main
    pthread
)

target_include_directories(router_optimization_test PRIVATE
    ../include
)

# 添加测试到CTest
if(BUILD_TESTING)
    enable_testing()
    add_test(NAME RouterOptimizationIntegrationTest 
             COMMAND router_optimization_integration_test)
    add_test(NAME RouterOptimizationTest 
             COMMAND router_optimization_test)
endif()

# 性能测试目标
add_custom_target(run_optimization_tests
    COMMAND ./router_optimization_integration_test --gtest_output=xml:optimization_test_results.xml
    DEPENDS router_optimization_integration_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "运行路由器优化测试 - Running router optimization tests"
)

# 创建测试报告目标
add_custom_target(test_report
    COMMAND echo "=== 路由器优化测试报告 Router Optimization Test Report ==="
    COMMAND echo "测试二进制文件位置 Test binary location: ${CMAKE_CURRENT_BINARY_DIR}/router_optimization_integration_test"
    COMMAND echo "运行测试 Run tests: make run_optimization_tests"
    COMMAND echo "或直接运行 Or run directly: ./router_optimization_integration_test"
    COMMENT "显示测试信息 - Display test information"
) 