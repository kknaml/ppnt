
add_library(boringssl-wrapper OBJECT src/mod.cpp)

target_sources(boringssl-wrapper PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES src/mod.ixx
)

target_link_libraries(boringssl-wrapper ssl crypto pthread)

target_include_directories(boringssl-wrapper PUBLIC "../boringssl/include")