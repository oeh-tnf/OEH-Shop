set(CARDLOGIN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/cardlogin.cpp
    )

add_executable(cardlogin ${CARDLOGIN_SRCS})

target_link_libraries(cardlogin oehshop stdc++fs dl evdev)

install(TARGETS cardlogin
    RUNTIME DESTINATION /usr/bin)

set_property(TARGET cardlogin PROPERTY CXX_STANDARD 17)
