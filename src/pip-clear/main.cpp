#include "common/utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

static const std::vector<std::string> KEEP_PACKAGES = {
    "pyinstaller", "torch", "virtualenv", "pip", "setuptools", "wheel"
};

static std::string to_lower(const std::string& s) {
    std::string result = s;
    for (char& c : result) c = (char)std::tolower((unsigned char)c);
    return result;
}

static bool should_keep(const std::string& pkg_name) {
    std::string lower = to_lower(pkg_name);
    for (const auto& keep : KEEP_PACKAGES) {
        if (lower.find(keep) != std::string::npos) return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        println("用法: pip-clear [version]");
        println("示例: pip-clear");
        println("示例: pip-clear 3.11.2");
        return 1;
    }

    std::string ver;
    if (argc == 2) ver = argv[1];

    println("正在获取已安装的包列表...");

    std::vector<std::string> all_pkgs;
    std::vector<std::wstring> pip_args;
    if (!ver.empty()) pip_args.push_back(utf8_to_utf16(ver));
    pip_args.push_back(L"freeze");

    int ret = run_process_with_output(L"pip", pip_args,
        [&](const std::string& line) {
            size_t eq_pos = line.find("==");
            if (eq_pos != std::string::npos)
                all_pkgs.push_back(line.substr(0, eq_pos));
            else if (!line.empty() && line.find('#') != 0)
                all_pkgs.push_back(line);
        }
    );

    if (ret != 0) {
        print_error("pip freeze 命令失败");
        return ret;
    }

    std::vector<std::string> to_remove;
    for (const auto& pkg : all_pkgs) {
        if (!should_keep(pkg)) to_remove.push_back(pkg);
    }

    if (to_remove.empty()) {
        println("没有可卸载的包。");
        return 0;
    }

    println("即将卸载以下包：");
    for (const auto& name : to_remove)
        println("  - " + name);

    std::vector<std::wstring> uninstall_args;
    if (!ver.empty()) uninstall_args.push_back(utf8_to_utf16(ver));
    uninstall_args.push_back(L"uninstall");
    for (const auto& pkg : to_remove)
        uninstall_args.push_back(utf8_to_utf16(pkg));
    uninstall_args.push_back(L"-y");
    uninstall_args.push_back(L"--no-input");

    println("正在卸载...");
    int unret = run_process(L"pip", uninstall_args);
    if (unret == 0)
        println("清理完成");
    else
        print_error("卸载过程中出现错误，退出码: " + std::to_string(unret));
    return unret;
}
