# pytools — Windows 便携式 Python 版本管理器

一键安装、切换、管理多个 Python 嵌入式版本。**纯 C++ 实现，不依赖 Python 运行时。**

## 工具一览

| 命令 | 用法 | 作用 |
|------|------|------|
| **py** | `py <version> [args...]` | 启动指定版本 Python REPL 或运行脚本 |
| **pip** | `pip <version> <pip args...>` | 以指定版本运行 pip（自动清华镜像 + PyTorch CUDA 源） |
| **pyins** | `pyins <version>` | 下载并安装嵌入式 Python 到 `A:/py<version>` |
| **crenv** | `crenv <version> <env_name>` | 安装 virtualenv 并创建虚拟环境 |
| **install** | `install <version>` | 通过 pip 安装 `requirements.txt` 依赖 |
| **uninstall** | `uninstall <version>` | 通过 pip 卸载 `requirements.txt` 依赖 |
| **pip-clear** | `pip-clear [version]` | 清除非核心 pip 包（保留 pip/setuptools/wheel 等） |
| **dels** | `dels <path> [path...]` | 强制删除文件/目录（绕过只读/系统属性） |

## 快速开始

### 下载

从 [Releases](https://github.com/ycm50/pytools/releases) 下载 `pytools-v1.0.0-windows-x64.zip`，解压后加入 PATH。

### 安装 Python

```cmd
# 安装 Python 3.12.1 到 A:/py3.12.1
pyins 3.12.1

# 安装 Python 3.10.1
pyins 3.10.1
```

### 运行 Python

```cmd
# 启动 REPL
py 3.12.1

# 运行脚本
py 3.12.1 script.py

# 执行内联代码
py 3.12.1 -c "print('hello')"

# 查看版本
py 3.12.1 --version
```

### 管理包

```cmd
# 安装包（自动使用清华镜像 + PyTorch CUDA 源）
pip 3.12.1 install numpy torch

# 列出已安装包
pip 3.12.1 list

# 从 requirements.txt 安装
install 3.12.1

# 清除非核心包（保留 pip/setuptools/wheel）
pip-clear 3.12.1
```

### 虚拟环境

```cmd
# 创建虚拟环境
crenv 3.12.1 myenv

# 激活
myenv\Scripts\activate
```

### 盘符配置

默认安装到 `A:` 盘。通过环境变量更改：

```cmd
set PYTOOLS_DRIVE=D
pyins 3.12.1     # 安装到 D:/py3.12.1
py 3.12.1        # 使用 D:/py3.12.1/python.exe
```

## 从源码构建

### 依赖

- CMake ≥ 3.15
- C++17 编译器（MinGW GCC / MSVC）

### 编译

```cmd
cmake -B build
cmake --build build
```

指定盘符（可选）：

```cmd
cmake -B build -DPYTOOLS_DRIVE_LETTER=D
cmake --build build
```

编译产物在 `build/bin/`：

```
build/bin/
├── py.exe
├── pip.exe
├── pyins.exe
├── crenv.exe
├── install_tool.exe
├── uninstall_tool.exe
├── pip-clear.exe
└── dels.exe
```

## 项目结构

```
pytools/
├── CMakeLists.txt           # 顶层 CMake 构建
├── src/
│   ├── CMakeLists.txt       # 源码构建（静态库 + 8 个 exe）
│   ├── common/
│   │   ├── utils.h          # 共享 API
│   │   └── utils.cpp        # CreateProcess/下载/ZIP解压/强制删除
│   ├── py/main.cpp          # py 工具
│   ├── pip/main.cpp         # pip 工具
│   ├── pyins/main.cpp       # Python 安装工具
│   ├── crenv/main.cpp       # 虚拟环境创建工具
│   ├── install_tool/main.cpp  # 依赖安装工具
│   ├── uninstall_tool/main.cpp # 依赖卸载工具
│   ├── pip-clear/main.cpp   # pip 清理工具
│   └── dels/main.cpp        # 强制删除工具
└── build/                   # 编译输出（已加入 .gitignore）
```
