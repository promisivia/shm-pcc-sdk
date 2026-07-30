add_library(clevel-hash INTERFACE)
target_include_directories(clevel-hash INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(clevel-hash INTERFACE shm-lib)
