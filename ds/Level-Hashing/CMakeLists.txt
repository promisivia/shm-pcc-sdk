cmake_minimum_required(VERSION 3.10)

project(levelhash)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# set(CMAKE_CXX_COMPILER /usr/bin/g++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g -Wall -Wno-unused-variable -lm")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,/usr/local/lib")

# file(GLOB_RECURSE BENCHMARK_SOURCES "benchmark/*.{cpp,c}")
# file(GLOB TEST_SOURCES "test/*.cpp")
# include_directories(../shm/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# ${TEST_SOURCES} ${BENCHMARK_SOURCES}

set(SRC_FILE hash.cc hash.h level_hashing.cc level_hashing.h spinlock.h)

add_library(levelhash STATIC ${SRC_FILE})
target_link_libraries(levelhash shm-lib)
target_include_directories(levelhash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)

# add_executable(clevel ycsb.cc ${SRC_FILE})

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "clevel;*.o;liblevelhash.a")