set(PAM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/module.c)

add_library(printingstation_pam SHARED ${PAM_SRCS})
