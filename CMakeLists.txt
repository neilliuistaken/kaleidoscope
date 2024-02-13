cmake_minimum_required(VERSION 3.16)

project(kale)

set(CMAKE_CXX_STANDARD 17)

include_directories("/usr/include/llvm-14")
include_directories("/usr/include/llvm-c-14")

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    add_compile_options(-fstandalone-debug)
endif()

include_directories(include)

add_subdirectory(src)
