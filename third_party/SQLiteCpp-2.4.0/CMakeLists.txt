# Main CMake file for compiling the library itself, examples and tests.
#
# Copyright (c) 2012-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)
cmake_minimum_required(VERSION 2.8.12) # first version with add_compile_options()
if (CMAKE_VERSION VERSION_LESS 3.0)
    project(SQLiteCpp)
    set(PROJECT_VERSION_MAJOR 2)
    set(PROJECT_VERSION_MINOR 4)
    set(PROJECT_VERSION_PATCH 0)
    set(PROJECT_VERSION "2.4.0")
else()
    cmake_policy(SET CMP0048 NEW)
    project(SQLiteCpp VERSION "2.4.0")
endif()

message (STATUS "CMake version: ${CMAKE_VERSION}")

# Define useful variables to handle OS differences:
if (WIN32)
    set(DEV_NULL                "NUL")
else (WIN32) # UNIX
    set(DEV_NULL                "/dev/null")
endif (WIN32)

# then Compiler/IDE differences:
if (MSVC)
    set(CPPLINT_ARG_OUTPUT      "--output=vs7")
    set(CPPCHECK_ARG_TEMPLATE   "--template=vs")
    # disable Visual Studio warnings for fopen() used in the example
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # Flags for linking with multithread static C++ runtime, required by googletest
    if (SQLITECPP_BUILD_TESTS)
        message(STATUS "Linking against multithread static C++ runtime for unit tests with googletest")
        set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} /MT")
        set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} /MTd")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    endif (SQLITECPP_BUILD_TESTS)
    # Handle the (partly supported) MSVC versions prior to 2015
    if (MSVC_VERSION LESS 1900) # OR MSVC_TOOLSET_VERSION LESS 140)
      message(WARNING "MSVC < 2015 detected: Visual Studio prior to 2015 is not fully supported. BLOB storage seems to be corrupted.")
    endif (MSVC_VERSION LESS 1900)
else (MSVC)
    set(CPPLINT_ARG_OUTPUT      "--output=eclipse")
    set(CPPCHECK_ARG_TEMPLATE   "--template=gcc")
    # Useful compile flags and extra warnings
    add_compile_options(-fstack-protector -Wall -Wextra -Wpedantic -Wno-long-long -Wswitch-enum -Wshadow -Winline)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++0x-compat") # C++ only
    if (CMAKE_COMPILER_IS_GNUCXX)
        # GCC flags
        option(SQLITECPP_USE_GCOV "USE GCov instrumentation." OFF)
        if (SQLITECPP_USE_GCOV)
            message (STATUS "Using GCov instrumentation")
            add_compile_options (-coverage) # NOTE -fkeep-inline-functions would be usefull but not working with current google test and gcc 4.8
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -coverage")
        endif ()
    endif (CMAKE_COMPILER_IS_GNUCXX)
endif (MSVC)
# and then common variables
set(CPPLINT_ARG_VERBOSE "--verbose=3")
set(CPPLINT_ARG_LINELENGTH "--linelength=120")

# Print CXX compiler information
message (STATUS "CMAKE_CXX_COMPILER '${CMAKE_CXX_COMPILER}' '${CMAKE_CXX_COMPILER_ID}' '${CMAKE_CXX_COMPILER_VERSION}'")

# Print CXX FLAGS
message (STATUS "CMAKE_CXX_FLAGS                '${CMAKE_CXX_FLAGS}'")
if (MSVC)
    message (STATUS "CMAKE_CXX_FLAGS_DEBUG          '${CMAKE_CXX_FLAGS_DEBUG}'")
    message (STATUS "CMAKE_CXX_FLAGS_RELEASE        '${CMAKE_CXX_FLAGS_RELEASE}'")
    message (STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO '${CMAKE_CXX_FLAGS_RELWITHDEBINFO}'")
    message (STATUS "CMAKE_CXX_FLAGS_MINSIZEREL     '${CMAKE_CXX_FLAGS_MINSIZEREL}'")
else (NOT MSVC)
    if     (CMAKE_BUILD_TYPE STREQUAL Debug)
        message (STATUS "CMAKE_CXX_FLAGS_DEBUG          '${CMAKE_CXX_FLAGS_DEBUG}'")
    elseif (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
        message (STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO '${CMAKE_CXX_FLAGS_RELWITHDEBINFO}'")
    elseif (CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
        message (STATUS "CMAKE_CXX_FLAGS_MINSIZEREL     '${CMAKE_CXX_FLAGS_MINSIZEREL}'")
    else ()
        message (STATUS "CMAKE_CXX_FLAGS_RELEASE        '${CMAKE_CXX_FLAGS_RELEASE}'")
    endif ()
endif ()

# Options relative to SQLite and SQLiteC++ functions

option(SQLITE_ENABLE_COLUMN_METADATA "Enable Column::getColumnOriginName(). Require support from sqlite3 library." ON)
if (SQLITE_ENABLE_COLUMN_METADATA)
    # Enable the use of SQLite column metadata and Column::getColumnOriginName() method,
    # Require that the sqlite3 library is also compiled with this flag (default under Debian/Ubuntu, but not on Mac OS X).
    add_definitions(-DSQLITE_ENABLE_COLUMN_METADATA)
endif (SQLITE_ENABLE_COLUMN_METADATA)

option(SQLITE_ENABLE_ASSERT_HANDLER "Enable the user defintion of a assertion_failed() handler." OFF)
if (SQLITE_ENABLE_ASSERT_HANDLER)
    # Enable the user defintion of a assertion_failed() handler (default to false, easier to handler for begginers).
    add_definitions(-DSQLITECPP_ENABLE_ASSERT_HANDLER)
endif (SQLITE_ENABLE_ASSERT_HANDLER)

option(SQLITE_USE_LEGACY_STRUCT "Fallback to forward declaration of legacy struct sqlite3_value (pre SQLite 3.19)" OFF)
if (SQLITE_USE_LEGACY_STRUCT)
    # Force forward declaration of legacy struct sqlite3_value (pre SQLite 3.19)
    add_definitions(-DSQLITE_USE_LEGACY_STRUCT)
endif (SQLITE_USE_LEGACY_STRUCT)


## Build the C++ Wrapper ##

# adding a new file require explicittly modifing the CMakeLists.txt
# so that CMake knows that it should rebuild the project (it is best practice)

# list of sources files of the library
set(SQLITECPP_SRC
 ${PROJECT_SOURCE_DIR}/src/Backup.cpp
 ${PROJECT_SOURCE_DIR}/src/Column.cpp
 ${PROJECT_SOURCE_DIR}/src/Database.cpp
 ${PROJECT_SOURCE_DIR}/src/Exception.cpp
 ${PROJECT_SOURCE_DIR}/src/Statement.cpp
 ${PROJECT_SOURCE_DIR}/src/Transaction.cpp
 ${PROJECT_SOURCE_DIR}/src/sqlite3.c
)
source_group(src FILES ${SQLITECPP_SRC})

# list of header files of the library
set(SQLITECPP_INC
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/SQLiteCpp.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Assertion.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Backup.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Column.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Database.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Exception.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Statement.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Transaction.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/Utils.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/VariadicBind.h
 ${PROJECT_SOURCE_DIR}/include/SQLiteCpp/ExecuteMany.h
)
source_group(include FILES ${SQLITECPP_INC})

# list of test files of the library
set(SQLITECPP_TESTS
 tests/Column_test.cpp
 tests/Database_test.cpp
 tests/Statement_test.cpp
 tests/Backup_test.cpp
 tests/Transaction_test.cpp
 tests/VariadicBind_test.cpp
 tests/Exception_test.cpp
 tests/ExecuteMany_test.cpp
)
source_group(tests FILES ${SQLITECPP_TESTS})

# list of example files of the library
set(SQLITECPP_EXAMPLES
 examples/example1/main.cpp
)
source_group(example1 FILES ${SQLITECPP_EXAMPLES})

# list of doc files of the library
set(SQLITECPP_DOC
 README.md
 LICENSE.txt
 CHANGELOG.md
 TODO.txt
)
source_group(doc FILES ${SQLITECPP_DOC})

# list of script files of the library
set(SQLITECPP_SCRIPT
 .travis.yml
 appveyor.yml
 build.bat
 build.sh
 cpplint.py
 Doxyfile
 FindSQLiteCpp.cmake
)
source_group(scripts FILES ${SQLITECPP_SCRIPT})

# All includes are relative to the "include" directory
include_directories("${PROJECT_SOURCE_DIR}/include")

# add sources of the wrapper as a "SQLiteCpp" static library
add_library(SQLiteCpp STATIC ${SQLITECPP_SRC} ${SQLITECPP_INC} ${SQLITECPP_DOC} ${SQLITECPP_SCRIPT})
# make the sqlite3 library part of the interface of the SQLiteCpp wrapper itself (the client app does not need to link to sqlite3)
# PR https://github.com/SRombauts/SQLiteCpp/pull/111 "linked SQLiteCpp to sqlite3" commented out since it breaks install step from PR #118
# target_link_libraries(SQLiteCpp PUBLIC sqlite3)

if (UNIX AND (CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))
    set_target_properties(SQLiteCpp PROPERTIES COMPILE_FLAGS "-fPIC")
endif (UNIX AND (CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"))

target_include_directories(SQLiteCpp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/>)

## Build provided copy of SQLite3 C library ##

# TODO 
#find_package(sqlite3)
#if(sqlite3_VERSION VERSION_LESS "3.19")
#    set_target_properties(SQLiteCpp PROPERTIES COMPILE_FLAGS "-DSQLITECPP_HAS_MEM_STRUCT")
#endif()

option(SQLITECPP_USE_ASAN "Use Address Sanitizer." OFF)
if (SQLITECPP_USE_ASAN)
    if ((CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 6) OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
        message (STATUS "Using Address Sanitizer")
        set_target_properties(SQLiteCpp PROPERTIES COMPILE_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        if (CMAKE_COMPILER_IS_GNUCXX)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
        endif ()
    endif ()
endif (SQLITECPP_USE_ASAN)

# Optional additional targets:

option(SQLITECPP_RUN_CPPLINT "Run cpplint.py tool for Google C++ StyleGuide." ON)
if (SQLITECPP_RUN_CPPLINT)
    find_package(PythonInterp)
    if (PYTHONINTERP_FOUND)
        # add a cpplint target to the "all" target
        add_custom_target(SQLiteCpp_cpplint
         ALL
         COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/cpplint.py ${CPPLINT_ARG_OUTPUT} ${CPPLINT_ARG_VERBOSE} ${CPPLINT_ARG_LINELENGTH} ${SQLITECPP_SRC} ${SQLITECPP_INC}
        )
    endif (PYTHONINTERP_FOUND)
else (SQLITECPP_RUN_CPPLINT)
    message(STATUS "SQLITECPP_RUN_CPPLINT OFF")
endif (SQLITECPP_RUN_CPPLINT)

