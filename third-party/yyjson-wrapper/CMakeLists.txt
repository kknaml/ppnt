
add_library(yyjson-wrapper OBJECT "src/mod.cpp" "src/yyjson.cpp")

target_sources(yyjson-wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)

target_link_libraries(yyjson-wrapper PUBLIC ppnt-common)
target_include_directories(yyjson-wrapper PUBLIC "include")

