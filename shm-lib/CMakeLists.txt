cmake_minimum_required(VERSION 3.10)
project(shm-lib)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找numa库
find_library(NUMA_LIB numa)

# 查找ssh库
find_package(Libssh REQUIRED)

# 根据架构设置编译器标志
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i386|i686")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -msse4.1 -g -Wall")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
endif()

# 包含头文件目录
include_directories(include)

# 添加源文件
file(GLOB_RECURSE SOURCES
    "clstale/*.cc"
    "msg/*.cc"
    "shm/*.cc"
    "utils/*.cc"
    "connection/*.cc"
)

# 添加静态库
add_library(shm-lib STATIC ${SOURCES})

target_include_directories(shm-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)