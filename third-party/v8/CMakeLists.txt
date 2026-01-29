

add_library(v8 STATIC src/mod.cpp)

target_sources(v8 PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)

target_include_directories(v8 PUBLIC
        "include"
        "${V8_OUT_DIR}/gen/include"
)

target_link_libraries(v8
        PRIVATE
        v8_monolith
        pthread
        dl
)

add_library(v8_rust_shim OBJECT "src/v8_rust_shim.cpp")