#include "common/utils.h"
#include "common/json.h"

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
#include <thread>
#include <fstream>

// ============================================================
//  盘符
// ============================================================

std::wstring normalize_drive(const std::wstring& input) {
    std::wstring d;
    for (wchar_t c : input) {
        if (c == L' ' || c == L'\t' || c == L'"' || c == L'\'')
            continue;
        d += c;
    }
    if (d.empty())
        return L"";

    // 统一 "/" 为 "\"
    for (wchar_t& c : d) {
        if (c == L'/') c = L'\\';
    }

    // "A:" / "A:\" / "A:\xxx" -> "A:"
    if (d.size() >= 2 && d[1] == L':') {
        wchar_t letter = d[0];
        if ((letter >= L'a' && letter <= L'z') || (letter >= L'A' && letter <= L'Z'))
            return std::wstring(1, (wchar_t)towupper(letter)) + L":";
        return L"";
    }

    // "A" -> "A:"
    if (d.size() == 1) {
        wchar_t letter = d[0];
        if ((letter >= L'a' && letter <= L'z') || (letter >= L'A' && letter <= L'Z'))
            return std::wstring(1, (wchar_t)towupper(letter)) + L":";
        return L"";
    }

    return L"";
}

std::wstring current_working_drive() {
    DWORD len = GetCurrentDirectoryW(0, nullptr);
    if (len == 0)
        return L"";
    std::wstring buf(len, L'\0');
    DWORD written = GetCurrentDirectoryW(len, &buf[0]);
    if (written == 0 || written >= len)
        return L"";
    buf.resize(written);
    return normalize_drive(buf);
}

std::wstring resolve_drive_letter(std::wstring* error) {
    // 1. 环境变量 PYTOOLS_DRIVE 显式指定
    wchar_t env[64] = {0};
    DWORD ret = GetEnvironmentVariableW(L"PYTOOLS_DRIVE", env, 64);
    if (ret > 0 && ret < 64) {
        std::wstring d = normalize_drive(env);
        if (!d.empty())
            return d;
    }

    // 2. 自动识别：当前工作目录所在盘
    std::wstring cwd_drive = current_working_drive();
    if (!cwd_drive.empty() && directory_exists(drive_root(cwd_drive)))
        return cwd_drive;

    // 3. config.json 中记录的盘符（当前目录不是普通盘路径时兜底）
    ConfigData cfg;
    if (load_config(cfg)) {
        std::wstring d = normalize_drive(cfg.drive);
        if (!d.empty() && directory_exists(drive_root(d)))
            return d;
    }

    // 4. 编译期默认盘符
    std::wstring fallback = normalize_drive(L"" PYTOOLS_DRIVE_LETTER);
    if (!fallback.empty() && directory_exists(drive_root(fallback)))
        return fallback;

    if (error) {
        std::wstring msg = L"无法识别当前工作目录所在盘符";
        if (!cwd_drive.empty())
            msg += L"（" + cwd_drive + L" 盘不存在）";
        *error = msg;
    }
    return L"";
}

std::wstring get_drive_letter() {
    std::wstring drive = resolve_drive_letter();
    if (!drive.empty())
        return drive;
    return normalize_drive(L"" PYTOOLS_DRIVE_LETTER);
}

std::wstring drive_root(const std::wstring& drive) {
    std::wstring d = normalize_drive(drive);
    if (d.empty())
        return L"";
    return d + L"\\";
}

// ============================================================
//  配置文件（与二进制同目录的 config.json）
// ============================================================

std::wstring get_executable_path() {
    std::vector<wchar_t> buf(MAX_PATH);
    for (;;) {
        DWORD n = GetModuleFileNameW(nullptr, buf.data(), (DWORD)buf.size());
        if (n == 0)
            return L"";
        if (n < buf.size()) {
            buf.resize(n);
            return std::wstring(buf.begin(), buf.end());
        }
        buf.resize(buf.size() * 2);
    }
}

std::wstring get_executable_dir() {
    std::wstring exe = get_executable_path();
    size_t pos = exe.find_last_of(L"/\\");
    if (pos == std::wstring::npos)
        return L".";
    return exe.substr(0, pos);
}

std::wstring get_config_path() {
    return get_executable_dir() + L"\\config.json";
}

bool load_config(ConfigData& cfg) {
    std::ifstream in(get_config_path().c_str(), std::ios::binary);
    if (!in)
        return false;
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    if (text.empty())
        return false;

    jsonx::Value root;
    if (!jsonx::parse(text, root) || !root.is_object())
        return false;

    cfg = ConfigData{};
    if (const jsonx::Value* v = root.find("drive"))
        cfg.drive = normalize_drive(utf8_to_utf16(v->as_string()));
    if (const jsonx::Value* v = root.find("install_root"))
        cfg.install_root = utf8_to_utf16(v->as_string());

    if (const jsonx::Value* v = root.find("pythons")) {
        if (v->type == jsonx::Value::Object) {
            for (const auto& kv : v->object) {
                if (!kv.second.is_string())
                    continue;
                cfg.pythons.emplace_back(utf8_to_utf16(kv.first),
                                         utf8_to_utf16(kv.second.as_string()));
            }
        }
    }
    return true;
}

bool save_config(const ConfigData& cfg) {
    std::wstring path = get_config_path();
    std::wstring dir = get_executable_dir();
    if (!directory_exists(dir))
        create_directories(dir);

    std::string out;
    out += "{\n";
    out += "  \"drive\": " + jsonx::quote(utf16_to_utf8(cfg.drive)) + ",\n";
    out += "  \"install_root\": " + jsonx::quote(utf16_to_utf8(cfg.install_root)) + ",\n";
    out += "  \"pythons\": {";

    std::vector<std::pair<std::wstring, std::wstring>> entries = cfg.pythons;
    std::sort(entries.begin(), entries.end(),
              [](const std::pair<std::wstring, std::wstring>& a,
                 const std::pair<std::wstring, std::wstring>& b) {
                  return a.first < b.first;
              });

    if (!entries.empty()) {
        out += "\n";
        for (size_t i = 0; i < entries.size(); ++i) {
            out += "    " + jsonx::quote(utf16_to_utf8(entries[i].first)) + ": "
                 + jsonx::quote(utf16_to_utf8(entries[i].second));
            if (i + 1 < entries.size()) out += ",";
            out += "\n";
        }
        out += "  ";
    }
    out += "}\n";
    out += "}\n";

    std::ofstream f(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!f)
        return false;
    f.write(out.data(), (std::streamsize)out.size());
    f.close();
    return f ? true : (f.bad() ? false : true);
}

// 从 python.exe 的完整路径推导出它所在的盘符与安装根目录
// 例如 "C:\py3.11.1\python.exe" -> drive="C:"、install_root="C:\"
static bool drive_from_exe(const std::wstring& exe_path,
                           std::wstring& drive, std::wstring& install_root) {
    if (exe_path.size() < 2 || exe_path[1] != L':')
        return false;
    wchar_t letter = exe_path[0];
    if (!((letter >= L'a' && letter <= L'z') || (letter >= L'A' && letter <= L'Z')))
        return false;
    drive = std::wstring(1, (wchar_t)towupper(letter)) + L":";
    install_root = drive + L"\\";
    return true;
}

bool record_python_install(const std::wstring& version, const std::wstring& python_exe) {
    ConfigData cfg;
    if (!load_config(cfg)) {
        cfg = ConfigData{};
    }

    // 以实际安装路径为准，避免 -InstallDir 跨盘等情况下 drive/install_root 与实际不符
    std::wstring exe_drive, exe_root;
    if (drive_from_exe(python_exe, exe_drive, exe_root)) {
        cfg.drive = exe_drive;
        cfg.install_root = exe_root;
    } else {
        cfg.drive = get_drive_letter();
        if (cfg.install_root.empty())
            cfg.install_root = drive_root(cfg.drive);
    }

    bool replaced = false;
    for (auto& kv : cfg.pythons) {
        if (kv.first == version) {
            kv.second = python_exe;
            replaced = true;
            break;
        }
    }
    if (!replaced)
        cfg.pythons.emplace_back(version, python_exe);

    return save_config(cfg);
}

bool unrecord_python_install(const std::wstring& version) {
    ConfigData cfg;
    if (!load_config(cfg))
        return false;

    bool removed = false;
    for (auto it = cfg.pythons.begin(); it != cfg.pythons.end();) {
        if (it->first == version) {
            it = cfg.pythons.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    if (!removed)
        return false;
    return save_config(cfg);
}

// ============================================================
//  临时目录
// ============================================================

// 展开字符串中内嵌的 %VAR% 环境变量引用
static std::wstring expand_env_vars(const std::wstring& input) {
    if (input.find(L'%') == std::wstring::npos)
        return input;

    std::wstring out;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] != L'%') {
            out += input[i++];
            continue;
        }
        size_t end = input.find(L'%', i + 1);
        if (end == std::wstring::npos) {
            out += input.substr(i);
            break;
        }
        std::wstring name = input.substr(i + 1, end - i - 1);
        if (name.empty()) {
            out += L'%';
            i = end + 1;
            continue;
        }
        DWORD need = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
        if (need == 0) {
            // 变量不存在，原样保留
            out += input.substr(i, end - i + 1);
        } else {
            std::wstring value(need, L'\0');
            DWORD written = GetEnvironmentVariableW(name.c_str(), &value[0], need);
            if (written == 0 || written >= need)
                out += input.substr(i, end - i + 1);
            else {
                value.resize(written);
                out += value;
            }
        }
        i = end + 1;
    }
    return out;
}

static std::wstring trim_trailing_separators(std::wstring path) {
    while (!path.empty() && (path.back() == L'\\' || path.back() == L'/'))
        path.pop_back();
    // 保留根路径形式（"C:" 或 "\\server\share" 除外）
    if (path.size() == 2 && path[1] == L':')
        path += L'\\';
    return path;
}

// 读取环境变量指向的目录，存在才采用
static bool temp_dir_from_env(const wchar_t* name, std::wstring& out) {
    DWORD need = GetEnvironmentVariableW(name, nullptr, 0);
    if (need <= 1)
        return false;
    std::wstring value(need, L'\0');
    DWORD written = GetEnvironmentVariableW(name, &value[0], need);
    if (written == 0 || written >= need)
        return false;
    value.resize(written);

    std::wstring expanded = trim_trailing_separators(expand_env_vars(value));
    if (expanded.empty() || !directory_exists(expanded))
        return false;
    out = expanded;
    return true;
}

// 动态获取临时目录：优先 %TEMP%，其次 %TMP%，都不行再交给系统 API
std::wstring get_temp_dir() {
    std::wstring dir;
    if (temp_dir_from_env(L"TEMP", dir))
        return dir;
    if (temp_dir_from_env(L"TMP", dir))
        return dir;

    DWORD len = GetTempPathW(0, nullptr);
    if (len == 0)
        return L"";
    std::wstring buf(len, L'\0');
    DWORD written = GetTempPathW(len, &buf[0]);
    if (written == 0 || written > len)
        return L"";
    buf.resize(written);
    return trim_trailing_separators(buf);
}

std::wstring create_temp_dir(const std::wstring& prefix) {
    std::wstring base = get_temp_dir();
    if (base.empty())
        return L"";

    for (int i = 0; i < 32; ++i) {
        wchar_t guid[64] = {0};
        GUID id = {};
        if (FAILED(CoCreateGuid(&id)))
            return L"";
        if (StringFromGUID2(id, guid, 64) == 0)
            return L"";
        std::wstring name = prefix + L"-" + guid;
        for (wchar_t& c : name) {
            if (c == L'{' || c == L'}' || c == L'-')
                c = L'_';
        }
        std::wstring path = base + L"\\" + name;
        if (CreateDirectoryW(path.c_str(), nullptr))
            return path;
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            return L"";
    }
    return L"";
}

// ============================================================
//  路径
// ============================================================

bool is_absolute_path(const std::wstring& path) {
    if (path.size() >= 2 && path[1] == L':')
        return true;
    if (path.size() >= 2 && (path[0] == L'\\' || path[0] == L'/'))
        return true;
    return false;
}

std::wstring get_python_path(const std::wstring& version) {
    ConfigData cfg;
    if (load_config(cfg)) {
        for (const auto& kv : cfg.pythons) {
            if (kv.first == version && !kv.second.empty())
                return kv.second;
        }
    }
    return get_drive_letter() + L"/py" + version + L"/python.exe";
}

std::wstring get_python_scripts_dir(const std::wstring& version) {
    std::wstring python = get_python_path(version);
    size_t pos = python.find_last_of(L"/\\");
    if (pos == std::wstring::npos)
        return L"";
    return python.substr(0, pos) + L"\\Scripts";
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
        if (!arg.empty() && (arg.find(L' ') != std::wstring::npos ||
                             arg.find(L'\t') != std::wstring::npos)) {
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
    // 不设置 STARTF_USESTDHANDLES，子进程直接继承当前控制台，
    // 交互式程序（REPL、pip 进度条）可以正常显示和接收输入。
    si.dwFlags = 0;

    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(0);

    BOOL ok = CreateProcessW(nullptr, buf.data(), nullptr, nullptr, TRUE,
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
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWr;
    si.hStdError  = hStderrWr;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

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
                        if (SUCCEEDED(hr)) {
                            bool ok = wait_for_extract(dest_dir);
                            pItems->Release();
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

// 注意：全部使用 DeleteFileW / RemoveDirectoryW，文件不进入回收站
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

static std::function<void()>& exit_cleanup_slot() {
    static std::function<void()> fn;
    return fn;
}

void set_exit_cleanup(std::function<void()> cleanup) {
    exit_cleanup_slot() = std::move(cleanup);
}

void die(const std::string& msg) {
    print_error(msg);
    // exit() 不会调用栈上对象的析构函数，这里先执行注册的清理回调
    auto& slot = exit_cleanup_slot();
    if (slot) {
        auto fn = slot;
        slot = nullptr;
        fn();
    }
    exit(1);
}
