# ─── 公共库 ──────────────────────────────────────────────────
add_library(pytools_common STATIC
    common/utils.cpp
)

target_include_directories(pytools_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pytools_common PUBLIC ${WIN_LIBS})

# ─── 辅助宏：创建单个工具可执行文件 ──────────────────────────
function(add_pytool NAME)
    add_executable(${NAME} ${NAME}/main.cpp)
    target_link_libraries(${NAME} PRIVATE pytools_common)
    set_target_properties(${NAME} PROPERTIES
        OUTPUT_NAME "${NAME}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endfunction()

# ─── 所有工具 ────────────────────────────────────────────────
add_pytool(py)
add_pytool(pip)
add_pytool(pyins)
add_pytool(crenv)
add_pytool(install_tool)
add_pytool(uninstall_tool)
add_pytool(pip-clear)
add_pytool(dels)

# 安装规则
install(TARGETS
    py pip pyins crenv install_tool uninstall_tool pip-clear dels
    RUNTIME DESTINATION bin
)
