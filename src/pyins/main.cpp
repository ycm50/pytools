#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <objbase.h>

#include "common/utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

static bool is_semver(const std::string& s) {
    int dots = 0;
    for (char c : s) {
        if (c == '.') ++dots;
        else if (c < '0' || c > '9') return false;
    }
    return dots >= 2;
}

static void usage() {
    println("用法: pyins <version> [--force]");
    println("示例: pyins 3.11.5");
    println("");
    println("说明: 自动识别当前工作目录所在盘，把嵌入式 Python 安装到该盘根目录");
    println("      例如当前目录在 D 盘，则安装到 D:\\py<version>");
    println("      安装完成后把 python.exe 路径写入本程序同目录的 config.json");
    println("      安装位置可通过环境变量 PYTOOLS_DRIVE 强制指定，如 set PYTOOLS_DRIVE=E");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    std::string ver;
    bool force = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force" || arg == "-f") {
            force = true;
        } else if (arg == "--help" || arg == "-h" || arg == "/?") {
            usage();
            return 0;
        } else if (ver.empty()) {
            ver = arg;
        } else {
            usage();
            return 1;
        }
    }

    if (ver.empty() || !is_semver(ver))
        die("错误: 版本号格式无效，应为 x.y.z 格式");

    // ── 自动识别盘符 ──────────────────────────────────────────
    std::wstring drive_error;
    std::wstring drive = resolve_drive_letter(&drive_error);
    if (drive.empty()) {
        die("错误: " + utf16_to_utf8(drive_error) +
            "，可用环境变量 PYTOOLS_DRIVE 指定盘符（如 set PYTOOLS_DRIVE=E）");
    }

    std::wstring install_root = drive_root(drive);
    std::wstring version_w = utf8_to_utf16(ver);
    std::wstring target_dir = install_root + L"py" + version_w;
    std::wstring python_exe = target_dir + L"\\python.exe";

    println("识别到盘符: " + utf16_to_utf8(drive));
    println("安装目录: " + utf16_to_utf8(target_dir));

    // ── 已安装检测 ────────────────────────────────────────────
    if (file_exists(python_exe) && !force) {
        println("Python " + ver + " 已安装，跳过下载（如需重装请加 --force）");
        if (record_python_install(version_w, python_exe))
            println("已更新配置: " + utf16_to_utf8(get_config_path()));
        else
            print_error("写入 config.json 失败: " + utf16_to_utf8(get_config_path()));
        return 0;
    }

    if (force && directory_exists(target_dir)) {
        println("--force: 正在清理旧安装目录...");
        force_delete_directory(target_dir);
    }

    // 中间文件统一放在 %TEMP% 的临时目录，函数退出时（无论成功失败）整个删除，不进回收站
    std::wstring temp_dir = create_temp_dir(L"pytools-pyins");
    if (temp_dir.empty())
        die("错误: 无法创建临时目录（%TEMP%）");
    struct TempDirGuard {
        std::wstring path;
        ~TempDirGuard() {
            if (!path.empty()) {
                force_delete_directory(path);
                // 正常返回路径：注销回调，避免重复清理
                set_exit_cleanup(nullptr);
            }
        }
    } temp_guard{temp_dir};
    // die() 走 exit()，不会触发上方的析构函数，这里再注册一份兜底
    set_exit_cleanup([dir = temp_dir]() { force_delete_directory(dir); });

    std::string url = "https://www.python.org/ftp/python/" + ver + "/python-" + ver + "-embed-amd64.zip";
    std::wstring zip_path = temp_dir + L"\\python-" + version_w + L"-embed-amd64.zip";

    // 1. 下载
    println("正在下载 Python " + ver + " 压缩包...");
    println("  URL: " + url);
    if (!download_file(utf8_to_utf16(url), zip_path))
        die("下载失败");
    println("  下载完成");

    // 2. 解压
    println("正在解压到 " + utf16_to_utf8(target_dir) + "...");
    create_directories(target_dir);
    if (!extract_zip(zip_path, target_dir))
        die("解压失败");

    // 3. 修改 ._pth 文件
    std::wstring search = target_dir + L"\\python*._pth";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        std::wstring pth_path = target_dir + L"\\" + fd.cFileName;
        FindClose(hFind);
        println("正在修改 " + utf16_to_utf8(fd.cFileName) + " 以启用 site...");

        HANDLE hFile = CreateFileW(pth_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(hFile, nullptr);
            std::string content;
            if (size > 0) {
                std::vector<char> buf(size + 1, 0);
                DWORD read = 0;
                ReadFile(hFile, buf.data(), size, &read, nullptr);
                content.assign(buf.data(), read);
            }

            // 逐行检查是否有未注释的 import site
            bool has_active = false;
            size_t pos = 0;
            while (pos < content.size()) {
                size_t eol = content.find('\n', pos);
                if (eol == std::string::npos) eol = content.size();
                std::string line = content.substr(pos, eol - pos);
                size_t start = line.find_first_not_of(" \t\r");
                if (start != std::string::npos && line.substr(start) == "import site") {
                    has_active = true;
                    break;
                }
                pos = eol + 1;
            }

            if (!has_active) {
                content += "\r\nimport site\r\n";
                SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
                DWORD written = 0;
                WriteFile(hFile, content.data(), (DWORD)content.size(), &written, nullptr);
                SetEndOfFile(hFile);
                println("  已添加 import site");
            } else {
                println("  import site 已存在，跳过");
            }
            CloseHandle(hFile);
        }
    } else {
        println("警告: 未找到 ._pth 文件，跳过 site 配置");
    }

    // 4. 写入 config.json，供 py/pip/crenv 等其他二进制定位 Python
    if (record_python_install(version_w, python_exe)) {
        println("已将 Python 路径写入配置: " + utf16_to_utf8(get_config_path()));
    } else {
        print_error("写入 config.json 失败: " + utf16_to_utf8(get_config_path()));
    }

    // 5. 下载 get-pip.py
    size_t dot2 = ver.find('.');
    std::string major = ver.substr(0, dot2);
    std::string minor_rest = ver.substr(dot2 + 1);
    int minor = std::stoi(minor_rest.substr(0, minor_rest.find('.')));
    std::string pip_url = (minor >= 9)
        ? "https://bootstrap.pypa.io/get-pip.py"
        : "https://bootstrap.pypa.io/pip/" + major + "." + std::to_string(minor) + "/get-pip.py";

    std::wstring get_pip_path = temp_dir + L"\\get-pip.py";
    println("正在下载 get-pip.py...");
    if (!download_file(utf8_to_utf16(pip_url), get_pip_path)) {
        print_error("get-pip.py 下载失败，pip 需要手动安装");
        println("Python 嵌入式安装完成（无 pip）");
        return 1;
    }
    println("  get-pip.py 下载完成");

    // 6. 安装 pip
    println("正在安装 pip...");
    int ret = run_process(python_exe, { get_pip_path });
    if (ret == 0) {
        println("pip 安装完成");
    } else {
        print_error("pip 安装失败，退出码: " + std::to_string(ret));
    }

    println("Python " + ver + " 嵌入式安装完成（便携版）");
    println("安装路径: " + utf16_to_utf8(target_dir));
    println("配置文件: " + utf16_to_utf8(get_config_path()));
    return ret;
}
