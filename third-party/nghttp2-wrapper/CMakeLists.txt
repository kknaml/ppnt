
add_library(nghttp2-wrapper OBJECT "src/mod.cpp")

target_sources(nghttp2-wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)

target_link_libraries(nghttp2-wrapper PUBLIC ppnt-common nghttp2)
target_include_directories(nghttp2-wrapper PUBLIC "../nghttp2/lib/includes")

set(USER_LINK_LIBS nghttp2-wrapper)
include(${PROJECT_SOURCE_DIR}/build_bins.cmake)
