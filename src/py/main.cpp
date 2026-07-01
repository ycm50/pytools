#include "common/utils.h"
#include <vector>
#include <string>

static bool looks_like_version(const std::string& s) {
    if (s.empty() || s == "--help" || s == "-h" || s == "/?")
        return false;
    int dots = 0;
    for (char c : s) {
        if (c == '.') ++dots;
        else if (c < '0' || c > '9') return false;
    }
    return dots >= 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        println("用法: py <version> [命令...]");
        println("示例: py 3.11.2             启动 REPL");
        println("示例: py 3.11.2 script.py   运行脚本");
        println("示例: py 3.11.2 -c \"...\"    运行内联代码");
        return 1;
    }

    std::string version = argv[1];
    if (!looks_like_version(version))
        die("错误: 无效的版本号 \"" + version + "\"，应为 x.y[.z] 格式");

    std::wstring python_path = get_python_path(utf8_to_utf16(version));
    if (!file_exists(python_path))
        die("错误: Python " + version + " 未安装于 " + utf16_to_utf8(python_path));

    if (argc == 2) {
        println("=== 启动 Python " + version + " REPL ===");
        return run_process(python_path, {});
    }

    std::vector<std::wstring> args;
    for (int i = 2; i < argc; ++i)
        args.push_back(utf8_to_utf16(argv[i]));

    int ret = run_process(python_path, args);
    if (ret != 0)
        print_error("命令失败，退出码: " + std::to_string(ret));
    return ret;
}
