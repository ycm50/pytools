#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>

#include "common/utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

static bool looks_like_version(const std::string& s) {
    if (s.empty())
        return false;
    int dots = 0;
    for (char c : s) {
        if (c == '.') ++dots;
        else if (c < '0' || c > '9') return false;
    }
    return dots >= 2;
}

static void usage() {
    println("用法: pyunin <version> [--force]");
    println("      pyunin --all [--force]");
    println("示例: pyunin 3.12.1");
    println("      pyunin --all --force");
    println("");
    println("说明: 卸载由 pyins 安装的便携版 Python，并同步删除 config.json 中的记录");
    println("      安装目录取自 config.json；配置里没有时回退为 <当前盘>:\\py<version>");
    println("      --all 卸载全部版本（配置中的记录 + 各固定盘根目录下的 py<版本>）");
    println("      --force 或 -f 表示不询问，直接删除");
    println("      删除不会进入回收站");
}

// 目录名是否形如 py3.12.1 / py3.12
static bool is_py_version_dir_name(const std::wstring& name) {
    if (name.size() < 4)
        return false;
    if (name[0] != L'p' && name[0] != L'P')
        return false;
    if (name[1] != L'y' && name[1] != L'Y')
        return false;
    int dots = 0;
    bool has_digit = false;
    for (size_t i = 2; i < name.size(); ++i) {
        wchar_t c = name[i];
        if (c == L'.') { ++dots; continue; }
        if (c < L'0' || c > L'9') return false;
        has_digit = true;
    }
    return has_digit && dots >= 1 && dots <= 2;
}

// 校验并返回可安全删除的目录；不合法时返回空串
static std::wstring validated_dir(const std::wstring& version, const std::wstring& python_exe) {
    size_t slash = python_exe.find_last_of(L"/\\");
    if (slash == std::wstring::npos)
        return L"";
    std::wstring dir = python_exe.substr(0, slash);
    std::wstring leaf = dir.substr(dir.find_last_of(L"/\\") + 1);
    std::wstring expected = L"py" + version;
    if (_wcsicmp(leaf.c_str(), expected.c_str()) != 0)
        return L"";
    return dir;
}

// 只扫描「本 pytools 安装目录所在盘」根目录下的 py<版本> 目录。
// 不遍历其他盘，避免误删同一台机器上属于别处的 Python。
static void scan_local_drive(std::vector<std::pair<std::wstring, std::wstring>>& found) {
    std::wstring exe_dir = get_executable_dir();
    if (exe_dir.size() < 2 || exe_dir[1] != L':')
        return;

    wchar_t letter = exe_dir[0];
    if (!((letter >= L'a' && letter <= L'z') || (letter >= L'A' && letter <= L'Z')))
        return;
    std::wstring root = std::wstring(1, (wchar_t)towupper(letter)) + L":\\";
    if (GetDriveTypeW(root.c_str()) != DRIVE_FIXED)
        return;

    {
        std::wstring pattern = root + L"py*";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE)
            return;
        do {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                continue;
            std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..")
                continue;
            if (!is_py_version_dir_name(name))
                continue;
            std::wstring dir = root + name;
            std::wstring exe = dir + L"\\python.exe";
            if (!file_exists(exe))
                continue;
            found.emplace_back(name.substr(2), exe);
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
}

// 卸载单个版本，返回 true 表示处理成功
static bool uninstall_one(const std::wstring& version, const std::wstring& python_exe, bool force) {
    std::wstring target_dir = validated_dir(version, python_exe);
    if (target_dir.empty()) {
        print_error("配置中的路径不是 pyins 的安装目录，拒绝删除: " + utf16_to_utf8(python_exe));
        return false;
    }

    println("待卸载版本: " + utf16_to_utf8(version));
    println("安装目录:   " + utf16_to_utf8(target_dir));

    if (!directory_exists(target_dir)) {
        println("目录不存在，仅清理 config.json 记录");
        unrecord_python_install(version);
        return true;
    }

    if (!force) {
        print("确认删除该目录及其全部内容？[y/N] ");
        char buf[16] = {0};
        if (!std::cin.getline(buf, sizeof(buf)) || (buf[0] != 'y' && buf[0] != 'Y')) {
            println("已取消");
            return false;
        }
    }

    println("正在删除...");
    if (!force_delete_directory(target_dir)) {
        print_error("删除失败，可能有文件正在被占用: " + utf16_to_utf8(target_dir));
        return false;
    }
    println("目录已删除");
    unrecord_python_install(version);
    return true;
}

static int uninstall_all(bool force) {
    // 版本 -> python.exe
    std::vector<std::pair<std::wstring, std::wstring>> targets;

    ConfigData cfg;
    if (load_config(cfg)) {
        for (const auto& kv : cfg.pythons) {
            if (!kv.second.empty())
                targets.emplace_back(kv.first, kv.second);
        }
    }
    scan_local_drive(targets);

    // 按版本去重（配置优先）
    std::vector<std::pair<std::wstring, std::wstring>> uniq;
    for (const auto& t : targets) {
        bool exists = false;
        for (const auto& u : uniq) {
            if (u.first == t.first) { exists = true; break; }
        }
        if (!exists)
            uniq.push_back(t);
    }

    if (uniq.empty()) {
        println("没有找到由 pyins 安装的 Python，跳过");
        return 0;
    }

    println("共发现 " + std::to_string(uniq.size()) + " 个由 pyins 安装的 Python");
    if (!force) {
        print("确认全部卸载？[y/N] ");
        char buf[16] = {0};
        if (!std::cin.getline(buf, sizeof(buf)) || (buf[0] != 'y' && buf[0] != 'Y')) {
            println("已取消");
            return 1;
        }
    }

    int failed = 0;
    for (const auto& t : uniq) {
        println("");
        if (!uninstall_one(t.first, t.second, true))
            ++failed;
    }

    println("");
    if (failed == 0) {
        println("全部卸载完成");
        return 0;
    }
    print_error(std::to_string(failed) + " 个版本卸载失败");
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    std::string ver;
    bool force = false;
    bool all = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force" || arg == "-f") {
            force = true;
        } else if (arg == "--all" || arg == "-a") {
            all = true;
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

    if (all) {
        if (!ver.empty())
            die("错误: --all 不能与具体版本号同时使用");
        return uninstall_all(force);
    }

    if (ver.empty() || !looks_like_version(ver))
        die("错误: 版本号格式无效，应为 x.y.z 格式");

    std::wstring version_w = utf8_to_utf16(ver);
    std::wstring python_exe = get_python_path(version_w);

    if (!uninstall_one(version_w, python_exe, force))
        return 1;

    println("");
    println("Python " + ver + " 卸载完成");
    return 0;
}
