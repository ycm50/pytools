#include "common/utils.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        println("用法: crenv <version> <env_name>");
        println("示例: crenv 3.11.2 myenv");
        println("");
        println("说明: 为指定 Python 版本创建虚拟环境");
        return 1;
    }

    std::string ver = argv[1];
    std::string env_name = argv[2];

    bool has_dot = (ver.find('.') != std::string::npos);
    if (!has_dot) {
        env_name = argv[1];
        ver = argv[2];
    }

    println("正在为 Python " + ver + " 创建虚拟环境 " + env_name);

    std::wstring python_path = get_python_path(utf8_to_utf16(ver));
    if (!file_exists(python_path))
        die("错误: Python " + ver + " 未安装于 " + utf16_to_utf8(python_path) +
            "（可在目标盘执行 pyins " + ver + " 安装）");

    // 步骤 1: 安装 virtualenv
    println("正在安装 virtualenv...");
    int ret = run_process(python_path, { L"-m", L"pip", L"install", L"virtualenv" });

    if (ret != 0) {
        // 回退：通过同目录的 pip 工具
        println("python -m pip 方式失败，尝试 pip 工具...");
        std::wstring pip_tool = get_executable_dir() + L"\\pip.exe";
        if (!file_exists(pip_tool))
            pip_tool = L"pip";
        std::vector<std::wstring> pip_args = {
            utf8_to_utf16(ver), L"install", L"virtualenv"
        };
        ret = run_process(pip_tool, pip_args);
        if (ret != 0)
            die("virtualenv 安装失败");
    }
    println("virtualenv 安装成功");

    // 步骤 2: 创建虚拟环境
    std::wstring env_path = utf8_to_utf16(env_name);
    std::wstring virtualenv_path = get_python_scripts_dir(utf8_to_utf16(ver)) + L"\\virtualenv.exe";

    if (file_exists(virtualenv_path)) {
        println("正在通过 virtualenv.exe 创建虚拟环境...");
        ret = run_process(virtualenv_path, { env_path });
    } else {
        println("virtualenv.exe 未找到，尝试 python -m virtualenv...");
        ret = run_process(python_path, { L"-m", L"virtualenv", env_path });
    }

    if (ret == 0) {
        println("虚拟环境 " + env_name + " 创建成功");
        println("激活: " + env_name + "\\Scripts\\activate");
    } else {
        print_error("virtualenv 命令执行失败，退出码: " + std::to_string(ret));
    }
    return ret;
}
