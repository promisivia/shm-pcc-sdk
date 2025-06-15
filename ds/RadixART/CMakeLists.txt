cmake_minimum_required(VERSION 3.6)
project(ARTSynchronized)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release)
else()
    message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -march=native -g")

find_library(JemallocLib jemalloc)
find_library(TbbLib tbb)
find_library(MemkindLib memkind)
find_library(NumaLib numa)
find_package(Threads)

set(ART_FILES OptimisticLockCoupling/Tree.cpp ROWEX/Tree.cpp ART/Tree.cpp)

# # include shared memory related part
# include_directories("../")
# set(ART_FILES ${ART_FILES}
#     "../shm/mm.cc"
#     "../shm/memkind.cc" 
#     "../shm/mm.cc"
#     "../connection/establish.cc"
#     "../utils/init.cc"
# )

add_library(ARTSynchronized ${ART_FILES})
# add_subdirectory(../../src/shm ${CMAKE_BINARY_DIR}/shm)
target_link_libraries(ARTSynchronized ${TbbLib} ${JemallocLib} ${MemkindLib} ${CMAKE_THREAD_LIBS_INIT} ${NumaLib} shm-lib)
target_include_directories(ARTSynchronized PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# set(SOURCE example.cpp)
# add_executable(example ${SOURCE})
# target_link_libraries(example ARTSynchronized)