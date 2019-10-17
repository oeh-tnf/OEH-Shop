set(CLIENT_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_executable(print_client ${CLIENT_SRCS})

target_link_libraries(print_client fltk)
target_include_directories(print_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/fltk-1.3.5/)
target_include_directories(print_client PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../third_party/fltk-1.3.5/)
