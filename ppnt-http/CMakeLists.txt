file(GLOB_RECURSE PPNT_HTTP_SOURCES src/*.cpp test/*.c)
file(GLOB_RECURSE PPNT_HTTP_MODULES src/*.ixx)

add_library(ppnt-http STATIC ${PPNT_HTTP_SOURCES})
target_sources(
        ppnt-http PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES ${PPNT_HTTP_MODULES}
)

target_link_libraries(ppnt-http PUBLIC ppnt-io boringssl-wrapper llhttp-wrapper)

set(USER_LINK_LIBS ppnt-http)
include(${PROJECT_SOURCE_DIR}/build_bins.cmake)

