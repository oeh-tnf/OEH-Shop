cmake_minimum_required(VERSION 3.2)
project(OEH_SHOP_OS)

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
