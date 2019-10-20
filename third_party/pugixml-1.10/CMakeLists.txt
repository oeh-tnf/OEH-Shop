cmake_minimum_required(VERSION 3.0)

project(pugixml VERSION 1.10)

option(BUILD_TESTS "Build tests" OFF)
option(USE_VERSIONED_LIBDIR "Use a private subdirectory to install the headers and libs" OFF)
option(USE_POSTFIX "Use separate postfix for each configuration to make sure you can install multiple build outputs" OFF)

set(BUILD_DEFINES "" CACHE STRING "Build defines")

if(MSVC)
	option(STATIC_CRT "Use static CRT libraries" OFF)

	# Rewrite command line flags to use /MT if necessary
	if(STATIC_CRT)
		foreach(flag_var
				CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
				CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
			if(${flag_var} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
			endif(${flag_var} MATCHES "/MD")
		endforeach(flag_var)
	endif()
endif()

# Pre-defines standard install locations on *nix systems.
include(GNUInstallDirs)
mark_as_advanced(CLEAR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_INCLUDEDIR)

set(HEADERS src/pugixml.hpp src/pugiconfig.hpp)
set(SOURCES src/pugixml.cpp)

if(DEFINED BUILD_DEFINES)
	foreach(DEFINE ${BUILD_DEFINES})
		add_definitions("-D" ${DEFINE})
	endforeach()
endif()

set(LIBRARY pugixml-static)

add_library(pugixml-static STATIC ${HEADERS} ${SOURCES})

if(BUILD_SHARED_LIBS AND MSVC)
	target_compile_definitions(pugixml PRIVATE "PUGIXML_API=__declspec(dllexport)")
endif()

if(USE_VERSIONED_LIBDIR)
	# Install library into its own directory under LIBDIR
	set(INSTALL_SUFFIX /pugixml-${pugixml_VERSION})
endif()

foreach(TARGET ${LIBRARY})
	# Enable C++11 long long for compilers that are capable of it
	if(NOT ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} STRLESS 3.1 AND ";${CMAKE_CXX_COMPILE_FEATURES};" MATCHES ";cxx_long_long_type;")
	  target_compile_features(${TARGET} PUBLIC cxx_long_long_type)
	endif()

	set_target_properties(${TARGET} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

	target_include_directories(${TARGET} PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}${INSTALL_SUFFIX}>)

	if(USE_POSTFIX)
		set_target_properties(${TARGET} PROPERTIES DEBUG_POSTFIX "_d" MINSIZEREL_POSTFIX "_m" RELWITHDEBINFO_POSTFIX "_r")
	endif()
endforeach()

configure_file(scripts/pugixml.pc.in ${PROJECT_BINARY_DIR}/pugixml.pc @ONLY)
