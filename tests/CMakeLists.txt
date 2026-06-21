include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    FIND_PACKAGE_ARGS NAMES GTest
)

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    message-queue-lib-tests
    lock_free_queue_ut.cpp
    circular_writer_block_ut.cpp
    blocking_bounded_queue_ut.cpp
    priority_queue_ut.cpp
    circular_drop_oldest_queue_ut.cpp
)

target_link_libraries(
    message-queue-lib-tests
    GTest::gtest_main
    GTest::gmock_main
    message_queue
)

target_include_directories(message-queue-lib-tests PUBLIC ${PROJECT_SOURCE_DIR}/include)

include(GoogleTest)

gtest_discover_tests(message-queue-lib-tests)
