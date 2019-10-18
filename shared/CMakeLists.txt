add_subdirectory(src)
add_library(oehshop STATIC ${OEHSHOP_SRCS})
target_include_directories(oehshop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
