
# add_subdirectory(spdlog-wrapper)

add_subdirectory(v8)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libs" FORCE)
set(OPENSSL_NO_ASM ON)
add_subdirectory(boringssl)
add_subdirectory(boringssl-wrapper)

add_subdirectory(llhttp)
add_subdirectory(llhttp-wrapper)

set(ENABLE_LIB_ONLY ON CACHE BOOL "Enable lib only")
set(ENABLE_APP OFF CACHE BOOL "Disable apps" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
set(ENABLE_HPACK_TOOLS OFF CACHE BOOL "Disable hpack tools" FORCE)
set(ENABLE_PYTHON_BINDINGS OFF CACHE BOOL "Disable python" FORCE)
set(ENABLE_STATIC_LIB ON CACHE BOOL "Build static lib")
set(ENABLE_SHARED_LIB OFF CACHE BOOL "Build shared lib")
add_subdirectory(nghttp2)
add_subdirectory(nghttp2-wrapper)

add_subdirectory(liburing-wrapper)

add_subdirectory(thread_pool_wrapper)