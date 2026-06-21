include(FetchContent)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/v1.8.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

# add_executable(message-queue-benchmark
#
# )

target_link_libraries(message-queue-benchmark
    PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
    message_queue
)

target_include_directories(message-queue-benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
