
set(LIBURING_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../liburing")
set(LIBURING_SRC_DIR "${LIBURING_ROOT}/src")

set(LIBURING_SOURCES
        "${LIBURING_SRC_DIR}/setup.c"
        "${LIBURING_SRC_DIR}/queue.c"
        "${LIBURING_SRC_DIR}/register.c"
        "${LIBURING_SRC_DIR}/syscall.c"
        "${LIBURING_SRC_DIR}/version.c"
)

add_library(liburing OBJECT ${LIBURING_SOURCES})

target_include_directories(liburing PUBLIC
        "${LIBURING_SRC_DIR}/include"
        "${LIBURING_ROOT}"
)

target_compile_definitions(liburing PRIVATE
        _GNU_SOURCE
        _LARGEFILE_SOURCE
        _FILE_OFFSET_BITS=64
        LIBURING_INTERNAL
)

target_compile_options(liburing PRIVATE "-include" "${LIBURING_ROOT}/config-host.h")


add_library(liburing-wrapper OBJECT "src/mod.cpp")

target_sources(liburing-wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)


target_include_directories(liburing-wrapper PUBLIC "${LIBURING_SRC_DIR}/include")
target_link_libraries(liburing-wrapper PUBLIC liburing)
