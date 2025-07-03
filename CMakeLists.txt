cmake_minimum_required(VERSION 3.10)
project(http_router VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##设置生成clangd的配置文件
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_PREFIX_PATH "${LIBBASE_PATH}/lib/cmake")

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Enable testing
enable_testing()

find_package(GTest CONFIG REQUIRED)

include_directories(
    ${LIBBASE_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Define a single test executable that includes all test source files
set(TEST_TARGETS
    http_router_tests
)

# Add a single test executable with all test sources
add_executable(http_router_tests
    tests/http_router_test.cpp #1
    tests/http_router_performance_test.cpp #2
    tests/http_router_hybrid_test.cpp #3
    tests/http_router_adv_test.cpp #4
    tests/http_router_stress_test.cpp #5

    tests/router_optimization_integration_test.cpp #6
    tests/router_optimization_test.cpp #7
    
    tests/router_simple_test.cpp #8
    tests/simple_test.cpp #9
    tests/lambda_context_test.cpp #10

    tests/test_router_group.cpp #11
    tests/test_router.cpp #12
    tests/massive_lambda_performance_test.cpp #13
    tests/massive_lambda_performance_test_fixed.cpp #14
)

target_link_directories(${TEST_TARGETS}
    PUBLIC ${LIBBASE_PATH}/lib/x86_64-linux-gnu
)


# Link test executables with GTest
target_link_libraries(${TEST_TARGETS}
    PUBLIC
        GTest::gtest
        GTest::gtest_main
        pthread
        rt
    PUBLIC c++ c++abi 
        
)




