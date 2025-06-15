cmake_minimum_required(VERSION 3.10)
project(tl2 C)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wall -pthread")

# 检测平台
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i686")
    set(PLATFORM_SOURCE platform_x86.h)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "sparc")
    set(PLATFORM_SOURCE platform_sparc.h)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# 源文件
set(TL2_SOURCES
    tl2.c
    thread.c
    tmalloc.c
)

# 头文件
set(TL2_HEADERS
    tl2.h
    thread.h
    tmalloc.h
    util.h
    common.h
    platform.h
    stm.h
    ${PLATFORM_SOURCE}
)

# 创建共享库
add_library(tl2 SHARED ${TL2_SOURCES} ${TL2_HEADERS})

# 设置包含目录
target_include_directories(tl2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 设置安装路径
set(STM_INSTALL_DIR "${CMAKE_BINARY_DIR}/install" CACHE PATH "STM installation directory")

# 安装规则
install(TARGETS tl2
    LIBRARY DESTINATION ${STM_INSTALL_DIR}/lib
    ARCHIVE DESTINATION ${STM_INSTALL_DIR}/lib
)

# 安装头文件
install(FILES stm.h tl2.h thread.h tmalloc.h util.h common.h platform.h platform_x86.h platform_sparc.h DESTINATION ${STM_INSTALL_DIR}/include)