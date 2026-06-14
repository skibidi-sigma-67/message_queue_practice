add_library(circular_writer_block_headers INTERFACE)

target_include_directories(
    circular_writer_block_headers
    INTERFACE
    ${PROJECT_SOURCE_DIR}/lib
)

target_compile_features(circular_writer_block_headers INTERFACE cxx_std_23)
