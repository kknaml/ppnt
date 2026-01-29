

# 1. 定义 V8 路径 (请确保这个路径绝对正确！)
set(V8_ROOT $ENV{V8_ROOT})
set(V8_OUT_DIR $ENV{V8_OUT_DIR})

set(V8_OBJ_DIR "${V8_OUT_DIR}/obj")

# 2. 定义 V8 定制的 Rust Sysroot 路径 (关键修正)
#    (之前报错是因为用了错误的 rust-toolchain 路径)
set(V8_SYSROOT_DIR "${V8_OUT_DIR}/local_rustc_sysroot/lib/rustlib/x86_64-unknown-linux-gnu/lib")

# === 调试检查 ===
if(NOT EXISTS "${V8_OBJ_DIR}")
    message(FATAL_ERROR "V8 Object directory not found at: ${V8_OBJ_DIR}")
endif()
if(NOT EXISTS "${V8_SYSROOT_DIR}")
    message(FATAL_ERROR "V8 Rust Sysroot not found at: ${V8_SYSROOT_DIR}\n请检查是否编译了 Rust 支持，或者路径是否正确。")
endif()

# 3. 扫描：Sysroot 基础库 (std, core, alloc 等)
file(GLOB V8_STD_LIBS "${V8_SYSROOT_DIR}/*.rlib")

# 4. 扫描：V8 组件库 (Temporal, ICU 等)
file(GLOB_RECURSE V8_FEATURE_LIBS "${V8_OBJ_DIR}/third_party/rust/*.rlib")

# 5. 合并并过滤
set(RAW_RUST_LIBS ${V8_STD_LIBS} ${V8_FEATURE_LIBS})
set(V8_RUST_LIBS_LIST "")

foreach(lib_path ${RAW_RUST_LIBS})
    # 过滤掉测试库、宿主工具库、proc_macro
    if(NOT lib_path MATCHES "test" AND
            NOT lib_path MATCHES "gtest" AND
            NOT lib_path MATCHES "proc_macro" AND
            NOT lib_path MATCHES "host_build_tools")
        list(APPEND V8_RUST_LIBS_LIST "${lib_path}")
    endif()
endforeach()

# === 核心检查：如果列表为空，必须报错 ===
list(LENGTH V8_RUST_LIBS_LIST LIB_COUNT)
if(LIB_COUNT EQUAL 0)
    message(FATAL_ERROR "错误：没有扫描到任何 .rlib 文件！路径可能错了，或者 V8 还没编译 Rust 部分。")
else()
    message(STATUS "find ${LIB_COUNT}  Rust libs")
endif()

link_directories(
        "${V8_OUT_DIR}/obj"
)
