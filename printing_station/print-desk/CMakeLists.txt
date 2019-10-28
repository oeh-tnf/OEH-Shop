set(CLIENT_DESK_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    )

add_executable(client_desk ${CLIENT_DESK_SRCS})

target_link_libraries(client_desk fltk oehshop stdc++fs)
target_include_directories(client_desk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/fltk-1.3.5/)
target_include_directories(client_desk PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../third_party/fltk-1.3.5/)

install(TARGETS client_desk
    RUNTIME DESTINATION /usr/bin)
