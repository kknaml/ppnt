
add_library(spdlog-wrapper OBJECT "src/mod.cpp")

target_sources(spdlog-wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)

target_include_directories(spdlog-wrapper PUBLIC "../spdlog/include")
