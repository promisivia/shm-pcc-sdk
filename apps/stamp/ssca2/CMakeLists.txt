# 设置源文件
set(SOURCES
    alg_radix_smp.c
    computeGraph.c
    createPartition.c
    cutClusters.c
    findSubGraphs.c
    genScalData.c
    getStartLists.c
    getUserParameters.c
    globals.c
    ssca2.c
    ${CMAKE_SOURCE_DIR}/lib/mt19937ar.c
    ${CMAKE_SOURCE_DIR}/lib/random.c
    ${CMAKE_SOURCE_DIR}/lib/thread.c
)

# 设置编译定义
set(DEFINITIONS ENABLE_KERNEL1)

# 设置基准测试名称
set(BENCHMARK_NAME ssca2)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template)
