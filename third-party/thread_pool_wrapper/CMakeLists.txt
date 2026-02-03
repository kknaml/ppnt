

add_library(thread_pool_wrapper OBJECT "src/mod.cpp")

target_include_directories(thread_pool_wrapper PUBLIC
    "../thread_pool/include"
)

target_sources( thread_pool_wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)