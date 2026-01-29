file(GLOB_RECURSE PPNT_COMMON_SOURCES src/*.cpp test/*.c)
file(GLOB_RECURSE PPNT_COMMON_MODULES src/*.ixx)

add_library(ppnt-common STATIC ${PPNT_COMMON_SOURCES})
target_sources(
    ppnt-common PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES ${PPNT_COMMON_MODULES}
)

set(USER_LINK_LIBS ppnt-common v8 v8_monolith v8_rust_shim "-Wl,--start-group" ${V8_RUST_LIBS_LIST} "-Wl,--end-group" boringssl-wrapper pthread dl)
include(../build_bins.cmake)
