file(GLOB_RECURSE PPNT_IO_SOURCES src/*.cpp test/*.c)
file(GLOB_RECURSE PPNT_IO_MODULES src/*.ixx)

add_library(ppnt-io STATIC ${PPNT_IO_SOURCES})
target_sources(
        ppnt-io PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES ${PPNT_IO_MODULES}
)

target_link_libraries(ppnt-io PUBLIC ppnt-common liburing-wrapper boringssl-wrapper liburing)


set(USER_LINK_LIBS ppnt-io)
include(${PROJECT_SOURCE_DIR}/build_bins.cmake)

