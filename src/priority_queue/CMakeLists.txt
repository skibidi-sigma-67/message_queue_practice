add_library(message_queue_priority_queue
    queue.cpp
)

target_link_libraries(message_queue_priority_queue PUBLIC message_queue_core)
target_include_directories(message_queue_priority_queue PUBLIC ${CMAKE_SOURCE_DIR}/lib/priority_queue)