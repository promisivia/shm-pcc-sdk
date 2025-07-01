cmake_minimum_required(VERSION 3.10)

# 项目名称和C++标准
project(btreeolc-project)
set(CMAKE_CXX_STANDARD 11)

# 源文件目录和包含目录

# 源文件和对象文件
set(SRCS )
set(OBJS ${SRCS})

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pipe -g -Werror -Wall -Wunused-function -Wno-class-memaccess -Wno-unused-parameter -Wextra")

# 添加可执行文件和库
add_library(btreeolc STATIC ${OBJS})
target_link_libraries(btreeolc pthread shm-lib)
# add_executable(btbench ${SRCS})

# # 链接库
# target_link_libraries(btbench)

# 清理目标
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "btbench;*.o;libbtreeolc.a")