#pragma once

#include <string>
#include <vector>
#include <functional>

// ============================================================
//  路径和盘符
// ============================================================

std::wstring get_drive_letter();
std::wstring get_python_path(const std::wstring& version);

// ============================================================
//  文件和路径工具
// ============================================================

bool file_exists(const std::wstring& path);
bool directory_exists(const std::wstring& path);
bool create_directories(const std::wstring& path);

// ============================================================
//  进程管理
// ============================================================

int run_process(const std::wstring& app, const std::vector<std::wstring>& args);
int run_process_cmdline(const std::wstring& cmdline);

int run_process_with_output(
    const std::wstring& app,
    const std::vector<std::wstring>& args,
    std::function<void(const std::string&)> on_stdout_line,
    std::function<void(const std::string&)> on_stderr_line = nullptr
);

// ============================================================
//  网络下载
// ============================================================

bool download_file(const std::wstring& url, const std::wstring& dest_path);

// ============================================================
//  ZIP 解压
// ============================================================

bool extract_zip(const std::wstring& zip_path, const std::wstring& dest_dir);

// ============================================================
//  强制删除
// ============================================================

bool force_delete_file(const std::wstring& path);
bool force_delete_directory(const std::wstring& path);

// ============================================================
//  编码与控制台
// ============================================================

std::wstring utf8_to_utf16(const std::string& utf8);
std::string  utf16_to_utf8(const std::wstring& utf16);

void print(const std::string& msg);
void println(const std::string& msg);
void print_error(const std::string& msg);
[[noreturn]] void die(const std::string& msg);
