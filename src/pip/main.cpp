#include "common/utils.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        println("用法: pip <version> <pip 参数...>");
        println("示例: pip 3.11.2 install numpy");
        println("示例: pip 3.11.2 list");
        return 1;
    }

    std::string version = argv[1];
    std::wstring python_path = get_python_path(utf8_to_utf16(version));
    if (!file_exists(python_path))
        die("错误: Python 解释器未找到于 " + utf16_to_utf8(python_path));

    std::vector<std::wstring> args;
    args.push_back(L"-m");
    args.push_back(L"pip");

    for (int i = 2; i < argc; ++i)
        args.push_back(utf8_to_utf16(argv[i]));

    if (argc >= 3 && std::string(argv[2]) == "install") {
        args.push_back(L"--index-url");
        args.push_back(L"https://pypi.tuna.tsinghua.edu.cn/simple");
        args.push_back(L"--trusted-host");
        args.push_back(L"pypi.tuna.tsinghua.edu.cn");
        args.push_back(L"--extra-index-url");
        args.push_back(L"https://download.pytorch.org/whl/cu124");
    }

    int ret = run_process(python_path, args);
    if (ret != 0)
        print_error("pip 命令执行失败，退出码: " + std::to_string(ret));
    return ret;
}
