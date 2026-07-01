cmake_minimum_required(VERSION 3.15)
project(pytools
    VERSION 1.0.0
    DESCRIPTION "Windows Portable Python Version Manager"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── 编译选项 ────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /utf-8)
elseif(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 默认盘符，可通过 -DPYTOOLS_DRIVE_LETTER=x 覆盖
if(NOT DEFINED PYTOOLS_DRIVE_LETTER)
    set(PYTOOLS_DRIVE_LETTER "a")
endif()
add_compile_definitions(PYTOOLS_DRIVE_LETTER="${PYTOOLS_DRIVE_LETTER}")

# ─── 链接 Windows 系统库 ───────────────────────────────────
set(WIN_LIBS
    shell32
    ole32
    oleaut32
    urlmon
    user32
    advapi32
    shlwapi
)

# ─── 子目录 ──────────────────────────────────────────────────
add_subdirectory(src)
