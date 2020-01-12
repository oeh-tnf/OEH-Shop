set(CARDLOGIN_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/cardlogin.cpp
    )

add_executable(cardlogin ${CARDLOGIN_SRCS})

target_link_libraries(cardlogin oehshop stdc++fs dl)

install(TARGETS cardlogin
    RUNTIME DESTINATION /usr/bin)
