# CMake version and project name
cmake_minimum_required(VERSION 3.14)
project(NginxMemoryPool CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add an option to build examples, defaulting to ON
option(BUILD_EXAMPLES "Build the examples" ON)
# Add an option to build benchmarks, defaulting to ON
option(BUILD_BENCHMARKS "Build the benchmarks" ON)
# Add an option to build tests, defaulting to ON
option(BUILD_TESTS "Build the unit tests" ON)

# Create the memory pool library from the source files in src/
add_library(ngx_pool src/ngx_mem_pool.cc)

# Specify the public include directory for the library.
# This ensures that both the library itself and any code linking against it
# can find the headers in the 'include' directory.
target_include_directories(ngx_pool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# If the option is enabled, build the example executable
if(BUILD_EXAMPLES)
    # Add the example executable
    add_executable(memory_pool_demo examples/main.cc)

    # Link the demo executable against our library
    target_link_libraries(memory_pool_demo PRIVATE ngx_pool)
endif()

# If the option is enabled, build the benchmark executable
if(BUILD_BENCHMARKS)
    # Add the benchmark executable
    add_executable(memory_pool_benchmark benchmark/bench.cc)

    # Link the benchmark executable against our library
    target_link_libraries(memory_pool_benchmark PRIVATE ngx_pool)
endif()

# If the option is enabled, build the test executable
if(BUILD_TESTS)
    # This will download and configure GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    # Now, define our test executable and link it to GTest
    add_executable(pool_tests tests/test_pool.cc)
    target_link_libraries(pool_tests PRIVATE ngx_pool GTest::gtest_main)
    
    # Discover tests
    include(GoogleTest)
    gtest_discover_tests(pool_tests)
endif() 