#include "common/utils.h"
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        println("用法: uninstall <version>");
        println("示例: uninstall 3.11.2");
        return 1;
    }

    std::string ver = argv[1];
    std::wstring python_path = get_python_path(utf8_to_utf16(ver));
    if (!file_exists(python_path))
        die("错误: Python " + ver + " 未安装于 " + utf16_to_utf8(python_path));

    println("正在卸载依赖 (requirements.txt)...");

    std::vector<std::wstring> args = {
        L"-m", L"pip", L"uninstall",
        L"-r", L"requirements.txt", L"-y"
    };

    int ret = run_process(python_path, args);
    if (ret == 0)
        println("依赖卸载完成");
    else
        print_error("卸载失败，退出码: " + std::to_string(ret));
    return ret;
}
