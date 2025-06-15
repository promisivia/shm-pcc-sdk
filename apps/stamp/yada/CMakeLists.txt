# 设置源文件
set(SOURCES
    coordinate.c
    element.c
    mesh.c
    region.c
    yada.c
    ${CMAKE_SOURCE_DIR}/lib/avltree.c
    ${CMAKE_SOURCE_DIR}/lib/heap.c
    ${CMAKE_SOURCE_DIR}/lib/list.c
    ${CMAKE_SOURCE_DIR}/lib/mt19937ar.c
    ${CMAKE_SOURCE_DIR}/lib/pair.c
    ${CMAKE_SOURCE_DIR}/lib/queue.c
    ${CMAKE_SOURCE_DIR}/lib/random.c
    ${CMAKE_SOURCE_DIR}/lib/rbtree.c
    ${CMAKE_SOURCE_DIR}/lib/thread.c
    ${CMAKE_SOURCE_DIR}/lib/vector.c
)

# 设置编译定义
set(DEFINITIONS
    LIST_NO_DUPLICATES
    MAP_USE_AVLTREE
    SET_USE_RBTREE
)

# 设置基准测试名称
set(BENCHMARK_NAME yada)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template) 