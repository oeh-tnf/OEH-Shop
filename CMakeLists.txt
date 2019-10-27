cmake_policy(SET CMP0048 NEW)
cmake_minimum_required(VERSION 3.2)
project(OEHSHOP
    VERSION 0.2)

set(OEHSHOP_VERSION ${OEHSHOP_VERSION_MAJOR}.${OEHSHOP_VERSION_MINOR}.${OEHSHOP_VERSION_PATCH})

find_package(Threads)

# Add Dependencies
add_subdirectory(third_party)

add_subdirectory(shared)

# Printing Station only works on Linux
if(NOT WIN32)
    add_subdirectory(printing_station)

    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lstdc++fs ${CMAKE_CXX_STANDARD_LIBRARIES}")
else()
    set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif()

if(NOT (CMAKE_BUILD_TYPE MATCHES DEBUG))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -Os")
endif()

# Desk is multiplatform
if(NOT ONLY_PRINTING)
    add_subdirectory(desk)
endif()

# Packaging
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME ${OEHSHOP})
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_CONTACT "max.heisinger@oeh.jku.at")
set(CPACK_PACKAGE_VENDOR "https://github.com/oeh-tnf/OEH-Shop")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ÖH Shop")
set(CPACK_SOURCE_GENERATOR "TBZ2;TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/.git/;~$;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md)
set(CPACK_PACKAGE_FILE_NAME "OEHSHOP-v${VERSION}")
include(CPack)
