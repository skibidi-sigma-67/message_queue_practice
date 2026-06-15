cmake_minimum_required(VERSION 3.12)

project(
    message_queue_practice
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

add_subdirectory(src)

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
