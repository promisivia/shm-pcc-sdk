# 设置源文件
set(SOURCES
    coordinate.c
    grid.c
    labyrinth.c
    maze.c
    router.c
    ${CMAKE_SOURCE_DIR}/lib/list.c
    ${CMAKE_SOURCE_DIR}/lib/mt19937ar.c
    ${CMAKE_SOURCE_DIR}/lib/pair.c
    ${CMAKE_SOURCE_DIR}/lib/queue.c
    ${CMAKE_SOURCE_DIR}/lib/random.c
    ${CMAKE_SOURCE_DIR}/lib/vector.c
)

# 设置编译定义
set(DEFINITIONS USE_EARLY_RELEASE)

# 设置基准测试名称
set(BENCHMARK_NAME labyrinth)

# 包含通用模板
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt.template)

# 链接数学库
target_link_libraries(labyrinth m) 