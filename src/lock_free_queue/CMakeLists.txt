add_library(message_queue_policy_lock_free
    queue.cpp
)

target_link_libraries(message_queue_policy_lock_free PUBLIC message_queue_core)

target_include_directories(message_queue_policy_lock_free PUBLIC ${CMAKE_SOURCE_DIR}/lib/lock_free_queue)
