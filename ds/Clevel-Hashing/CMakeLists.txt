find_package(PkgConfig REQUIRED)
pkg_check_modules(PMEMOBJ REQUIRED libpmemobj)

add_library(clevel-hash INTERFACE)
target_include_directories(clevel-hash INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(clevel-hash INTERFACE shm-lib)

target_include_directories(clevel-hash INTERFACE ${LIBPMEMOBJ_INCLUDE_DIRS})
target_link_directories(clevel-hash INTERFACE ${LIBPMEMOBJ_LIBRARY_DIRS})
target_link_libraries(clevel-hash INTERFACE ${PMEMOBJ_LIBRARIES})
