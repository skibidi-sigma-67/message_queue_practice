add_library(message_queue_core INTERFACE)

target_include_directories(message_queue_core INTERFACE ${CMAKE_SOURCE_DIR}/lib)

file(GLOB src_subdirs LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*")

foreach(dir ${src_subdirs})
  if(IS_DIRECTORY "${dir}" AND EXISTS "${dir}/CMakeLists.txt")
    add_subdirectory("${dir}")
  endif()
endforeach()
