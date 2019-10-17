cmake_minimum_required(VERSION 3.2)
project(OEH_SHOP_OS)

# Add Dependencies
add_subdirectory(third_party)

# Printing Station only works on Linux
if(NOT WIN32)
    add_subdirectory(printing_station)
endif()

# Desk is multiplatform
add_subdirectory(desk)
