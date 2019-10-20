set(DESK_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/user_table.cpp
    )

add_executable(print_desk ${DESK_SRCS})

target_link_libraries(print_desk fltk oehshop SQLiteCpp)
target_include_directories(print_desk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/fltk-1.3.5/)
target_include_directories(print_desk PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../third_party/fltk-1.3.5/)
