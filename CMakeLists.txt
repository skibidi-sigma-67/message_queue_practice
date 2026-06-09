cmake_minimum_required(VERSION 3.12)

project(
    message_queue_practice
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)


include_directories(lib)

add_subdirectory(benchmark)

enable_testing()
add_subdirectory(tests)
