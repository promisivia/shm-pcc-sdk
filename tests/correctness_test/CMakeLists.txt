# Specify the minimum version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(CorrectnessTests)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set build type
set(CMAKE_BUILD_TYPE Release)

# Compiler flags
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i386|i686")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -latomic -mcx16 -march=native -msse4.1")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -latomic -march=native")
endif()

# Include directories
include_directories(include)
include_directories(../../ds)

# 设置全局编译标志，供子项目使用
set(NO_CC ON)
set(OPT_GC ON)
set(OPT_ROOT_READ ON)
set(OPT_NO_META ON)
set(OPT_BATCH_FLUSH ON)
set(USE_CXL ON)

# Add shm-lib subdirectory
add_subdirectory(../../shm-lib ${CMAKE_BINARY_DIR}/shm-lib)

# Add BwTree subdirectory - 参考tests/YCSB-C/CMakeLists.txt的写法
add_subdirectory(../../ds/BwTree ${CMAKE_BINARY_DIR}/BwTree)
list(APPEND public_link_libs bwtree)

# Add Clevel-Hashing subdirectory
add_subdirectory(../../ds/Clevel-Hashing ${CMAKE_BINARY_DIR}/clevel-hash)
list(APPEND public_link_libs clevel-hash)

# Add linearizability test for clevel_hash
add_executable(clevel_linearizability_test clevel_hash_linearizability_test.cpp)
target_include_directories(clevel_linearizability_test PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../../ds/Clevel-Hashing/include
)
# Link with the same libraries as other targets
target_link_libraries(clevel_linearizability_test 
    ${public_link_libs} pthread tbb atomic ssh numa memkind pmemobj pmem
)
target_compile_definitions(clevel_linearizability_test PRIVATE
    NO_CC
    OPT_NO_META
    OPT_BATCH_FLUSH
)
set_target_properties(clevel_linearizability_test PROPERTIES 
    ENABLE_EXPORTS ON
    POSITION_INDEPENDENT_CODE TRUE
)

# Add linearizability test for bwtree
add_executable(bwtree_linearizability_test bwtree_linearizability_test.cpp)
target_include_directories(bwtree_linearizability_test PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../../ds/BwTree/src
)
target_link_libraries(bwtree_linearizability_test 
    shm-lib pthread tbb atomic ssh numa memkind pmemobj pmem bwtree
)
target_compile_definitions(bwtree_linearizability_test PRIVATE 
    NO_CC
    OPT_GC
    OPT_ROOT_READ
    USE_CXL
)
set_target_properties(bwtree_linearizability_test PROPERTIES 
    ENABLE_EXPORTS ON
    POSITION_INDEPENDENT_CODE TRUE
)

# Add custom commands to copy executables to root directory
add_custom_command(TARGET clevel_linearizability_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:clevel_linearizability_test> 
    ${CMAKE_SOURCE_DIR}/clevel_linearizability_test
)

add_custom_command(TARGET bwtree_linearizability_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bwtree_linearizability_test> 
    ${CMAKE_SOURCE_DIR}/bwtree_linearizability_test
)

# Add tree visualization test for bwtree
add_executable(test_tree_visualization test_tree_visualization.cpp)
target_include_directories(test_tree_visualization PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ../../ds/BwTree/src
)
target_link_libraries(test_tree_visualization 
    shm-lib pthread tbb atomic ssh numa memkind pmemobj pmem bwtree
)
target_compile_definitions(test_tree_visualization PRIVATE 
    NO_CC
    OPT_GC
    OPT_ROOT_READ
    USE_CXL
)
set_target_properties(test_tree_visualization PROPERTIES 
    ENABLE_EXPORTS ON
    POSITION_INDEPENDENT_CODE TRUE
)

# Add custom commands to copy executables to root directory
add_custom_command(TARGET clevel_linearizability_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:clevel_linearizability_test> 
    ${CMAKE_SOURCE_DIR}/clevel_linearizability_test
)

add_custom_command(TARGET bwtree_linearizability_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bwtree_linearizability_test> 
    ${CMAKE_SOURCE_DIR}/bwtree_linearizability_test
)

add_custom_command(TARGET test_tree_visualization POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_tree_visualization> 
    ${CMAKE_SOURCE_DIR}/test_tree_visualization
)

# Print configuration info
message(STATUS "Correctness Tests CMake configuration:")
message(STATUS "  - clevel_linearizability_test")
message(STATUS "  - bwtree_linearizability_test")
message(STATUS "  - test_tree_visualization")
message(STATUS "  - Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - C++ standard: ${CMAKE_CXX_STANDARD}")
