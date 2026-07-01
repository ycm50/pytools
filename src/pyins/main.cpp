#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>

#include "common/utils.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

static std::wstring get_downloads_folder() {
    wchar_t* path = nullptr;
    GUID folder_id;
    const wchar_t guid_str[] = L"{3D644C9B-1FB8-4F30-9B45-F670235F79C0}";
    unsigned long d1;
    unsigned short d2, d3;
    unsigned int d4[8];
    swscanf_s(guid_str, L"{%08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x}",
              &d1, &d2, &d3, &d4[0], &d4[1], &d4[2], &d4[3], &d4[4], &d4[5], &d4[6], &d4[7]);
    folder_id.Data1 = d1;
    folder_id.Data2 = d2;
    folder_id.Data3 = d3;
    for (int i = 0; i < 8; ++i)
        folder_id.Data4[i] = (unsigned char)d4[i];

    HRESULT hr = SHGetKnownFolderPath(folder_id, 0, nullptr, &path);
    if (FAILED(hr) || !path) {
        wchar_t buf[MAX_PATH];
        GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
        return std::wstring(buf) + L"\\Downloads";
    }
    std::wstring result(path);
    CoTaskMemFree(path);
    return result;
}

static bool is_semver(const std::string& s) {
    int dots = 0;
    for (char c : s) {
        if (c == '.') ++dots;
        else if (c < '0' || c > '9') return false;
    }
    return dots >= 2;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        println("用法: pyins <version>");
        println("示例: pyins 3.11.5");
        return 1;
    }

    std::string ver = argv[1];
    if (!is_semver(ver))
        die("错误: 版本号格式无效，应为 x.y.z 格式");

    std::wstring drive = get_drive_letter();
    std::wstring target_dir = drive + L"/py" + utf8_to_utf16(ver);
    std::wstring downloads_path = get_downloads_folder();

    std::string url = "https://www.python.org/ftp/python/" + ver + "/python-" + ver + "-embed-amd64.zip";
    std::wstring zip_path = downloads_path + L"\\py" + utf8_to_utf16(ver) + L".zip";

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
    DeleteFileW(zip_path.c_str());

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

    // 4. 下载 get-pip.py
    size_t dot2 = ver.find('.');
    std::string major = ver.substr(0, dot2);
    std::string minor_rest = ver.substr(dot2 + 1);
    int minor = std::stoi(minor_rest.substr(0, minor_rest.find('.')));
    std::string pip_url = (minor >= 9)
        ? "https://bootstrap.pypa.io/get-pip.py"
        : "https://bootstrap.pypa.io/pip/" + major + "." + std::to_string(minor) + "/get-pip.py";

    std::wstring get_pip_path = target_dir + L"\\get-pip.py";
    println("正在下载 get-pip.py...");
    if (!download_file(utf8_to_utf16(pip_url), get_pip_path)) {
        print_error("get-pip.py 下载失败，pip 需要手动安装");
        println("Python 嵌入式安装完成（无 pip）");
        return 1;
    }
    println("  get-pip.py 下载完成");

    // 5. 安装 pip
    std::wstring python_exe = target_dir + L"\\python.exe";
    println("正在安装 pip...");
    int ret = run_process(python_exe, { get_pip_path });
    if (ret == 0) {
        println("pip 安装完成");
    } else {
        print_error("pip 安装失败，退出码: " + std::to_string(ret));
    }

    DeleteFileW(get_pip_path.c_str());
    println("Python " + ver + " 嵌入式安装完成（便携版）");
    println("安装路径: " + utf16_to_utf8(target_dir));
    return ret;
}
