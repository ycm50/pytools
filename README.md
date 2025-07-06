# pytools

**pytools** 是一个集合了多种 Python 环境及工具的便捷套件。  
仅主目录包含核心内容，子目录为环境配置相关，可忽略。

---

## 目录

- [主要功能与工具说明](#主要功能与工具说明)
- [快速开始](#快速开始)
- [文件说明](#文件说明)
- [其他说明](#其他说明)
- [贡献与反馈](#贡献与反馈)

---

## 主要功能与工具说明

| 文件/工具         | 用法示例                           | 说明                                                         |
|------------------|-----------------------------------|--------------------------------------------------------------|
| `build.exe`      | `build d`                         | d 为盘符（不带冒号），编译以下 exe 工具，并创建 py 安装文件夹 |
| `pyins.exe`      | `pyins py版本号`                  | 安装指定 Python 版本到通过 build.exe 选定的驱动器           |
| `crenv.exe`      | `crenv py版本号 虚拟环境名`        | 创建指定 Python 版本的虚拟环境                               |
| `pip.exe`        | `pip py版本号 pip命令`             | 用对应 Python 版本执行 pip 命令<br>例：`pip 3.11.2 install numpy` |
| `py.exe`         | `py py版本号 命令`                 | 用指定 Python 版本执行命令<br>例：`py 310 script.py`         |
| `env.exe`        | `env 虚拟环境名`                   | 进入已创建的虚拟环境                                         |
| `ps.bat`         | -                                 | 在 cmd 下启动 PowerShell                                     |
| `install.bat`<br>`install.ps1` | -                    | Windows 下的安装辅助脚本，可选用                             |
| `env.cpp`        | -                                 | 环境相关工具的 C++ 源码                                      |

---

## 快速开始

1. **环境搭建**  
   首先使用 `build.exe`，如 `build d`，d 为目标盘符。
2. **安装 Python**  
   使用 `pyins.exe` 安装所需 Python 版本。
3. **创建虚拟环境**  
   通过 `crenv.exe` 建立虚拟环境。
4. **其他操作**  
   其余工具可根据上表说明使用。

---

## 文件说明

- 子目录为 py 编译环境的补充内容，普通用户无需关注。
- 更多详细文件请前往 [GitHub 主目录](https://github.com/ycm50/pytools/tree/master)。

---

## 贡献与反馈

如有问题请提交 Issue，欢迎 PR！

---
