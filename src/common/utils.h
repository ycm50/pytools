#pragma once

#include <string>
#include <vector>
#include <functional>

// ============================================================
//  路径和盘符
// ============================================================

// 归一化为 "X:" 形式，失败返回空串
std::wstring normalize_drive(const std::wstring& input);

// 当前工作目录所在盘符，如 "A:"，失败返回空串
std::wstring current_working_drive();

// 解析盘符，优先级：环境变量 PYTOOLS_DRIVE > config.json > 当前工作目录 > 编译期默认
// 无法识别时返回空串并写入 error
std::wstring resolve_drive_letter(std::wstring* error = nullptr);

// 解析盘符，识别失败时回退到编译期默认盘符（兼容旧行为）
std::wstring get_drive_letter();

// "A:" -> "A:\"
std::wstring drive_root(const std::wstring& drive);

// 指定版本的 python.exe 路径（优先使用 config.json 记录）
std::wstring get_python_path(const std::wstring& version);

// 指定版本的 Scripts 目录（虚拟环境等工具使用）
std::wstring get_python_scripts_dir(const std::wstring& version);

// ============================================================
//  配置文件（与二进制同目录的 config.json）
// ============================================================

struct ConfigData {
    std::wstring drive;        // 例如 "A:"
    std::wstring install_root; // Python 安装根目录，例如 "A:\"
    // 版本号 -> python.exe 完整路径
    std::vector<std::pair<std::wstring, std::wstring>> pythons;
};

std::wstring get_executable_path();
std::wstring get_executable_dir();
std::wstring get_config_path();

bool load_config(ConfigData& cfg);
bool save_config(const ConfigData& cfg);

// 记录一个 Python 安装（写入 config.json，与已有内容合并）
// drive / install_root 由 python_exe 自身路径推导，与安装位置保持一致
bool record_python_install(const std::wstring& version, const std::wstring& python_exe);

// 从 config.json 中移除某个版本的记录，返回是否真的移除了
bool unrecord_python_install(const std::wstring& version);

// ============================================================
//  临时目录（下载/解压中间文件）
// ============================================================

// %TEMP% 目录，失败返回空串
std::wstring get_temp_dir();

// 在 %TEMP% 下创建唯一子目录，失败返回空串
std::wstring create_temp_dir(const std::wstring& prefix);

// 注册进程退出前的清理回调：die() 会在 exit() 之前执行它，
// 保证异常路径（exit 不会走栈上析构函数）也能清理临时文件。
// 传 nullptr 可取消注册。
void set_exit_cleanup(std::function<void()> cleanup);

// ============================================================
//  文件和路径工具
// ============================================================

bool is_absolute_path(const std::wstring& path);
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
