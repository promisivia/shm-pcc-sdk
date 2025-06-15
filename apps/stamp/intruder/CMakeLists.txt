# 设置源文件
set(SOURCES
    decoder.c
    detector.c
    dictionary.c
    intruder.c
    packet.c
    preprocessor.c
    stream.c
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
set(DEFINITIONS MAP_USE_RBTREE)

# 设置基准测试名称
set(BENCHMARK_NAME intruder)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template) 