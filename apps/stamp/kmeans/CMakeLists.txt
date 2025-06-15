# 设置源文件
set(SOURCES
    cluster.c
    common.c
    kmeans.c
    normal.c
    ${CMAKE_SOURCE_DIR}/lib/mt19937ar.c
    ${CMAKE_SOURCE_DIR}/lib/random.c
)

# 设置编译定义
set(DEFINITIONS OUTPUT_TO_STDOUT)

# 设置基准测试名称
set(BENCHMARK_NAME kmeans)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template)
