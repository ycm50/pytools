#include "common/utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <urlmon.h>
#include <comdef.h>
#include <shlwapi.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <system_error>
#include <memory>
#include <locale>
#include <codecvt>
#include <thread>

// ============================================================
//  盘符
// ============================================================

std::wstring get_drive_letter() {
    wchar_t env[4] = {0};
    DWORD ret = GetEnvironmentVariableW(L"PYTOOLS_DRIVE", env, 4);
    if (ret > 0 && ret <= 3) {
        std::wstring d(env);
        if (!d.empty() && (d.back() == L':' || d.back() == L'/'))
            d.pop_back();
        if (!d.empty())
            return d + L":";
    }
    return L"" PYTOOLS_DRIVE_LETTER L":";
}

// ============================================================
//  路径
// ============================================================

std::wstring get_python_path(const std::wstring& version) {
    return get_drive_letter() + L"/py" + version + L"/python.exe";
}

bool file_exists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool directory_exists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool create_directories(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return true;
    size_t pos = 0;
    std::wstring dir;
    while ((pos = path.find_first_of(L"/\\", pos)) != std::wstring::npos) {
        dir = path.substr(0, pos++);
        if (dir.empty()) continue;
        CreateDirectoryW(dir.c_str(), nullptr);
    }
    if (!path.empty())
        CreateDirectoryW(path.c_str(), nullptr);
    return directory_exists(path);
}

// ============================================================
//  进程管理
// ============================================================

static std::wstring build_command_line(const std::wstring& app,
                                       const std::vector<std::wstring>& args) {
    std::wstring cmd;
    if (app.find(L' ') != std::wstring::npos)
        cmd = L'"' + app + L'"';
    else
        cmd = app;
    for (const auto& arg : args) {
        cmd += L' ';
        if (arg.find(L' ') != std::wstring::npos || arg.find(L'\t') != std::wstring::npos) {
            cmd += L'"' + arg + L'"';
        } else {
            cmd += arg;
        }
    }
    return cmd;
}

int run_process(const std::wstring& app, const std::vector<std::wstring>& args) {
    std::wstring cmdline = build_command_line(app, args);
    return run_process_cmdline(cmdline);
}

int run_process_cmdline(const std::wstring& cmdline) {
    PROCESS_INFORMATION pi = {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(0);

    BOOL ok = CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
                              CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi);
    if (!ok) {
        DWORD err = GetLastError();
        print_error("CreateProcessW 失败，错误码: " + std::to_string(err));
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(exit_code);
}

// ── 带输出捕获的进程运行 ────────────────────────────────────

static void read_pipe_thread(HANDLE hPipe, std::function<void(const std::string&)> callback) {
    char buf[4096];
    DWORD read = 0;
    std::string leftover;
    while (ReadFile(hPipe, buf, sizeof(buf) - 1, &read, nullptr) && read > 0) {
        buf[read] = '\0';
        leftover += buf;
        size_t pos;
        while ((pos = leftover.find('\n')) != std::string::npos) {
            std::string line = leftover.substr(0, pos);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (callback) callback(line);
            leftover.erase(0, pos + 1);
        }
    }
    if (!leftover.empty() && callback)
        callback(leftover);
}

int run_process_with_output(
    const std::wstring& app,
    const std::vector<std::wstring>& args,
    std::function<void(const std::string&)> on_stdout_line,
    std::function<void(const std::string&)> on_stderr_line)
{
    std::wstring cmdline = build_command_line(app, args);

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    HANDLE hStderrRd = nullptr, hStderrWr = nullptr;

    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0) ||
        !CreatePipe(&hStderrRd, &hStderrWr, &sa, 0)) {
        print_error("创建管道失败");
        return -1;
    }

    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStderrRd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi = {};
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hStdoutWr;
    si.hStdError  = hStderrWr;
    si.wShowWindow = SW_HIDE;

    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(0);

    BOOL ok = CreateProcessW(nullptr, buf.data(), nullptr, nullptr, TRUE,
                              CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);
    CloseHandle(hStderrWr);

    if (!ok) {
        CloseHandle(hStdoutRd);
        CloseHandle(hStderrRd);
        DWORD err = GetLastError();
        print_error("CreateProcessW 失败，错误码: " + std::to_string(err));
        return -1;
    }

    std::thread t1(read_pipe_thread, hStdoutRd, on_stdout_line);
    std::thread t2(read_pipe_thread, hStderrRd, on_stderr_line ? on_stderr_line : on_stdout_line);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    t1.join();
    t2.join();

    CloseHandle(hStdoutRd);
    CloseHandle(hStderrRd);

    return static_cast<int>(exit_code);
}

// ============================================================
//  下载
// ============================================================

bool download_file(const std::wstring& url, const std::wstring& dest_path) {
    DeleteFileW(dest_path.c_str());
    HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), dest_path.c_str(), 0, nullptr);
    if (FAILED(hr)) {
        print_error("下载失败，HRESULT: " + std::to_string(hr));
        return false;
    }
    return file_exists(dest_path);
}

// ============================================================
//  ZIP 解压
// ============================================================

static bool wait_for_extract(const std::wstring& dir, int timeout_ms = 15000) {
    int waited = 0;
    const int step = 200;
    while (waited < timeout_ms) {
        WIN32_FIND_DATAW fd;
        std::wstring pattern = dir + L"\\*";
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            bool non_empty = false;
            do {
                std::wstring name = fd.cFileName;
                if (name != L"." && name != L"..") { non_empty = true; break; }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
            if (non_empty) return true;
        }
        Sleep(step);
        waited += step;
    }
    return false;
}

static bool extract_via_powershell(const std::wstring& zip_path,
                                   const std::wstring& dest_dir) {
    std::wstring cmd1 =
        L"powershell -NoProfile -Command \"Expand-Archive -Path '"
        + zip_path + L"' -DestinationPath '" + dest_dir + L"' -Force\"";
    int ret = run_process_cmdline(cmd1);
    if (ret == 0) return true;

    std::wstring cmd2 =
        L"powershell -NoProfile -Command \"& { Add-Type -AssemblyName System.IO.Compression.FileSystem;"
        L" [System.IO.Compression.ZipFile]::ExtractToDirectory('"
        + zip_path + L"', '" + dest_dir + L"') }\"";
    ret = run_process_cmdline(cmd2);
    return (ret == 0);
}

bool extract_zip(const std::wstring& zip_path, const std::wstring& dest_dir) {
    if (!create_directories(dest_dir))
        return false;

    // 策略 1: Shell.Application COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
        IShellDispatch* pShell = nullptr;
        hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER,
                              IID_IShellDispatch, (void**)&pShell);
        if (SUCCEEDED(hr) && pShell) {
            VARIANT zipVar;
            VariantInit(&zipVar);
            zipVar.vt = VT_BSTR;
            zipVar.bstrVal = SysAllocString(zip_path.c_str());
            Folder* pZipFolder = nullptr;
            hr = pShell->NameSpace(zipVar, &pZipFolder);
            VariantClear(&zipVar);
            if (SUCCEEDED(hr) && pZipFolder) {
                VARIANT destVar;
                VariantInit(&destVar);
                destVar.vt = VT_BSTR;
                destVar.bstrVal = SysAllocString(dest_dir.c_str());
                Folder* pDestFolder = nullptr;
                hr = pShell->NameSpace(destVar, &pDestFolder);
                VariantClear(&destVar);
                if (SUCCEEDED(hr) && pDestFolder) {
                    FolderItems* pItems = nullptr;
                    hr = pZipFolder->Items(&pItems);
                    if (SUCCEEDED(hr) && pItems) {
                        VARIANT itemVar;
                        VariantInit(&itemVar);
                        itemVar.vt = VT_DISPATCH;
                        itemVar.pdispVal = pItems;
                        VARIANT optVar;
                        VariantInit(&optVar);
                        optVar.vt = VT_I4;
                        optVar.lVal = 0x14;
                        hr = pDestFolder->CopyHere(itemVar, optVar);
                        VariantClear(&itemVar);
                        pItems->Release();
                        if (SUCCEEDED(hr)) {
                            bool ok = wait_for_extract(dest_dir);
                            pDestFolder->Release();
                            pZipFolder->Release();
                            pShell->Release();
                            CoUninitialize();
                            if (ok) return true;
                        } else { pItems->Release(); }
                    }
                    pDestFolder->Release();
                }
                pZipFolder->Release();
            }
            pShell->Release();
        }
        CoUninitialize();
    }

    // 策略 2: PowerShell
    println("COM 解压失败，尝试 PowerShell...");
    return extract_via_powershell(zip_path, dest_dir);
}

// ============================================================
//  强制删除
// ============================================================

bool force_delete_file(const std::wstring& path) {
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    for (int i = 0; i < 3; ++i) {
        if (DeleteFileW(path.c_str()))
            return true;
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
        Sleep(100);
    }
    std::wstring tmp = path + L".del_tmp";
    MoveFileExW(path.c_str(), tmp.c_str(), MOVEFILE_REPLACE_EXISTING);
    DeleteFileW(tmp.c_str());
    return !file_exists(path);
}

bool force_delete_directory(const std::wstring& path) {
    std::wstring search = path + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..") continue;
            std::wstring full = path + L"\\" + name;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                force_delete_directory(full);
            else
                force_delete_file(full);
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    for (int i = 0; i < 3; ++i) {
        if (RemoveDirectoryW(path.c_str()))
            return true;
        SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
        Sleep(100);
    }
    return !directory_exists(path);
}

// ============================================================
//  编码转换
// ============================================================

std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
    std::wstring result(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &result[0], len);
    return result;
}

std::string utf16_to_utf8(const std::wstring& utf16) {
    if (utf16.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), nullptr, 0, nullptr, nullptr);
    std::string result(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), &result[0], len, nullptr, nullptr);
    return result;
}

// ============================================================
//  控制台输出
// ============================================================

static void write_console(const std::string& msg, WORD color = 7) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    bool is_console = (GetConsoleMode(hOut, &mode) != FALSE);
    if (color != 7 && is_console)
        SetConsoleTextAttribute(hOut, color);
    DWORD written = 0;
    std::wstring wmsg = utf8_to_utf16(msg);
    if (!WriteConsoleW(hOut, wmsg.c_str(), (DWORD)wmsg.size(), &written, nullptr)) {
        WriteFile(hOut, msg.c_str(), (DWORD)msg.size(), &written, nullptr);
    }
    if (color != 7 && is_console)
        SetConsoleTextAttribute(hOut, 7);
}

void print(const std::string& msg) { write_console(msg); }
void println(const std::string& msg) { write_console(msg + "\n"); }
void print_error(const std::string& msg) { write_console("[错误] " + msg + "\n", 12); }

void die(const std::string& msg) {
    print_error(msg);
    exit(1);
}
