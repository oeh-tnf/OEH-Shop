add_subdirectory(pam)

set(USE_PAM ON)

add_subdirectory(display-manager)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(XNEST_DEBUG ON)
    target_compile_definitions(slim PUBLIC XNEST_DEBUG)
endif()
