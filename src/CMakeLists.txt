find_package(Threads REQUIRED)

add_library(message_queue
    policies/blocking_bounded_queue.cpp
    policies/circular_drop_oldest_queue.cpp
    policies/circular_writer_block.cpp
    policies/lock_free_queue.cpp
    policies/priority_queue.cpp
)

target_include_directories(message_queue PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(message_queue PUBLIC Threads::Threads)
