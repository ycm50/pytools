#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "common/utils.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        println("用法: dels <path> [path...]");
        println("示例: dels temp_folder");
        println("示例: dels C:\\Users\\name\\temp");
        println("注意: 至少需要一个路径参数");
        return 1;
    }

    int fail_count = 0;
    for (int i = 1; i < argc; ++i) {
        std::wstring target = utf8_to_utf16(argv[i]);
        wchar_t full_path[MAX_PATH];
        GetFullPathNameW(target.c_str(), MAX_PATH, full_path, nullptr);
        std::wstring path(full_path);

        DWORD attr = GetFileAttributesW(path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            print_error("路径不存在: " + std::string(argv[i]));
            ++fail_count;
            continue;
        }

        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            println("正在删除目录: " + utf16_to_utf8(path) + "...");
            if (force_delete_directory(path))
                println("  已删除目录");
            else {
                print_error("  删除目录失败: " + utf16_to_utf8(path));
                ++fail_count;
            }
        } else {
            println("正在删除文件: " + utf16_to_utf8(path) + "...");
            if (force_delete_file(path))
                println("  已删除文件");
            else {
                print_error("  删除文件失败: " + utf16_to_utf8(path));
                ++fail_count;
            }
        }
    }

    if (fail_count > 0) {
        print_error(std::to_string(fail_count) + " 个路径删除失败");
        return 1;
    }
    println("全部删除完成");
    return 0;
}
