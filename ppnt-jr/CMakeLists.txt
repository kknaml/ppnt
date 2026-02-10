file(GLOB_RECURSE PPNT_JR_SOURCES src/*.cpp test/*.c)
file(GLOB_RECURSE PPNT_JR_MODULES src/*.ixx)

add_library(ppnt-jr OBJECT ${PPNT_JR_SOURCES})
target_sources(
        ppnt-jr PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES ${PPNT_JR_MODULES}
)

target_link_libraries(ppnt-jr PUBLIC v8 ppnt-common)

set(USER_LINK_LIBS ppnt-common ppnt-jr v8 v8_monolith v8_rust_shim "-Wl,--start-group" ${V8_RUST_LIBS_LIST} "-Wl,--end-group" pthread dl)
include(../build_bins.cmake)