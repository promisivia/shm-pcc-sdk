# 设置源文件
set(SOURCES
    gene.c
    genome.c
    segments.c
    sequencer.c
    table.c
    ${CMAKE_SOURCE_DIR}/lib/bitmap.c
    ${CMAKE_SOURCE_DIR}/lib/hash.c
    ${CMAKE_SOURCE_DIR}/lib/hashtable.c
    ${CMAKE_SOURCE_DIR}/lib/pair.c
    ${CMAKE_SOURCE_DIR}/lib/random.c
    ${CMAKE_SOURCE_DIR}/lib/list.c
    ${CMAKE_SOURCE_DIR}/lib/mt19937ar.c
    ${CMAKE_SOURCE_DIR}/lib/vector.c
)

# 设置编译定义
set(DEFINITIONS
    LIST_NO_DUPLICATES
    CHUNK_STEP1=12
)

# 设置基准测试名称
set(BENCHMARK_NAME genome)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template)