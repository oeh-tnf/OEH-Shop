add_subdirectory(src)
add_library(oehshop STATIC ${OEHSHOP_SRCS})
target_include_directories(oehshop PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(oehshop nng pugixml)
target_include_directories(oehshop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nng-1.1.1/)
set_property(TARGET oehshop PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET oehshop PROPERTY CXX_STANDARD 17)
