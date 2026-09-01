cmake_minimum_required(VERSION 3.16)
project(erpc_lrpc LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Werror -O2)

add_library(lrpc STATIC lib/lrpc.c lib/switch_stack.S)
target_include_directories(lrpc PUBLIC include include/uapi)

add_executable(lrpc-publisher demo/publisher.c)
target_include_directories(lrpc-publisher PRIVATE demo)
target_link_libraries(lrpc-publisher PRIVATE lrpc)

add_executable(lrpc-shadow demo/shadow.c)
target_link_libraries(lrpc-shadow PRIVATE lrpc)

add_executable(erpc-lrpc-client demo/erpc_client.cc)
target_include_directories(erpc-lrpc-client PRIVATE demo)
target_link_libraries(erpc-lrpc-client PRIVATE lrpc)

enable_testing()
