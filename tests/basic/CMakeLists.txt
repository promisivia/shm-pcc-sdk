# Specify the minimum version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(basic-test)

find_library(NUMA_LIB numa)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -latomic -mcx16 -march=native -lmemkind -lnuma")

set(CMAKE_BUILD_TYPE Debug)

# # Include directories
# include_directories(include)

# Specify the source files
file(GLOB SUBSRCS "test.cpp")

# Define the executable
add_executable(test_basics ${SUBSRCS})


link_directories(/usr/local/lib)

# Link libraries
add_subdirectory(../../shm-lib ${CMAKE_BINARY_DIR}/shm-lib)
target_link_libraries(test_basics
    shm-lib
)

# Custom command to copy the executable to the root directory
add_custom_command(TARGET test_basics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_basics> ${CMAKE_SOURCE_DIR}/test_basics)


# add_executable(test test.cc)
