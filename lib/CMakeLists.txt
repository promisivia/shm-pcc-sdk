cmake_minimum_required(VERSION 3.10)
project(shm-lib)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找numa库
find_library(NUMA_LIB numa)

# 查找ssh库
find_package(Libssh REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -lmemkind -msse4.1 -g -Wall -lssh")

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