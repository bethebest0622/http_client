cmake_minimum_required(VERSION 3.10)

SET(CMAKE_C_COMPILER "/usr/bin/gcc")
SET(CMAKE_CXX_COMPILER "/usr/bin/g++")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Werror -Wextra -Wno-unused-parameter -finline-functions")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++2a -Wall -fPIC -Werror -Wextra -Wno-unused-parameter -finline-functions")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wextra -finline-functions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++2a -Wall -fPIC -Wextra -finline-functions")

set(CMAKE_CXX_CPPLINT "cpplint")

project(curl_util)
add_subdirectory(lib_src core_dir)
#add_subdirectory(bin_src)
