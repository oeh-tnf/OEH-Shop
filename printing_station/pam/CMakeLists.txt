set(PAM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/module.cpp
    )

add_library(printingstation SHARED ${PAM_SRCS})

set_property(TARGET printingstation PROPERTY CXX_STANDARD 17)

target_link_libraries(printingstation pam pam_misc pamc pugixml-static oehshop stdc++fs)
set_property(TARGET printingstation PROPERTY POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "-fno-stack-protector")

install(
    TARGETS printingstation
    LIBRARY DESTINATION /usr/lib/security)

install(
    FILES printingstation.sample
    DESTINATION /etc/pam.d/)
