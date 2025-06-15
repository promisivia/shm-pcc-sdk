# 设置源文件
set(SOURCES
    adtree.c
    bayes.c
    data.c
    learner.c
    net.c
    sort.c
    ${CMAKE_SOURCE_DIR}/lib/bitmap.c
    ${CMAKE_SOURCE_DIR}/lib/list.c
    ${CMAKE_SOURCE_DIR}/lib/mt19937ar.c
    ${CMAKE_SOURCE_DIR}/lib/queue.c
    ${CMAKE_SOURCE_DIR}/lib/random.c
    ${CMAKE_SOURCE_DIR}/lib/thread.c
    ${CMAKE_SOURCE_DIR}/lib/vector.c
)

# 设置编译定义
set(DEFINITIONS
    LIST_NO_DUPLICATES
    LEARNER_TRY_REMOVE
    LEARNER_TRY_REVERSE
)

# 设置基准测试名称
set(BENCHMARK_NAME bayes)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template)
