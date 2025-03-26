cmake_minimum_required(VERSION 3.10)
project(http_router VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##设置生成clangd的配置文件
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Enable testing
enable_testing()



find_package(GTest REQUIRED)

include_directories(
    ${XBASE_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Define test executables
set(TEST_TARGETS
    http_router_test
    http_router_cache_test
    http_router_hybrid_test
)

# Add main router test executable
add_executable(http_router_test
    tests/http_router_test.cpp
)

# Add router cache test executable
add_executable(http_router_cache_test
    tests/http_router_cache_test.cpp
)

# Add hybrid routing test executable
add_executable(http_router_hybrid_test
    tests/http_router_hybrid_test.cpp
)

# Configure link directories for all test targets
foreach(target ${TEST_TARGETS})
    target_link_directories(
        ${target}
        PRIVATE
            ${XBASE_PATH}/lib
    )

    # Link test executables with GTest
    target_link_libraries(${target}
        PRIVATE
            gtest
            gtest_main
            pthread
    )

    # Add test to CTest
    add_test(NAME ${target} COMMAND ${target})
endforeach()

# Install documentation
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/
        DESTINATION share/doc/http_router
        FILES_MATCHING PATTERN "*.md" PATTERN "*.html" PATTERN "*.pdf")


# 安装include 下所有的头文件 
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/http_router
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")


