
add_library(llhttp-wrapper OBJECT "src/mod.cpp")

target_sources(llhttp-wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)

target_link_libraries(llhttp-wrapper llhttp_static)

target_include_directories(llhttp-wrapper PUBLIC "../llhttp/include")