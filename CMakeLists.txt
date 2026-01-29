cmake_minimum_required(VERSION 4.1.2)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_MODULE_STD 1)

project(ppnt)

add_compile_definitions(
        V8_COMPRESS_POINTERS=1
        V8_31BIT_SMIS_ON_64BIT_ARCH=1
        V8_ENABLE_SANDBOX
)


include(find_v8_rust.cmake)

add_subdirectory(third-party)
add_subdirectory(ppnt-common)
add_subdirectory(ppnt-jr)
add_subdirectory(ppnt-http)
add_subdirectory(ppnt-io)
