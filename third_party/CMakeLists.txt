set(OPTION_BUILD_EXAMPLES OFF)
set(OPTION_BUILD_HTML_DOCUMENTATION OFF)
set(OPTION_BUILD_PDF_DOCUMENTATION OFF)
set(OPTION_USE_GL OFF)

add_subdirectory(fltk-1.3.5)

set(TMP_CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR})

set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nng-1.1.1)
add_subdirectory(nng-1.1.1)

add_subdirectory(pugixml-1.10)

set_property(TARGET pugixml-static PROPERTY POSITION_INDEPENDENT_CODE ON)

add_subdirectory(SQLiteCpp-2.4.0)
