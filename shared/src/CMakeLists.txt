set(OEHSHOP_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/message.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/user_db.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/requester.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/reply_service.cpp
    PARENT_SCOPE)

if(NOT WIN32)
    set(UNIX_ONLY_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/user.cpp
        PARENT_SCOPE
        )
endif()
