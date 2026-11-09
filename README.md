# pytools — Windows 便携式 Python 版本管理器

一键安装、切换、管理多个 Python 嵌入式版本。**纯 C++ 实现，不依赖 Python 运行时。**

## 工具一览

| 命令 | 用法 | 作用 |
|------|------|------|
| **py** | `py <version> [args...]` | 启动指定版本 Python REPL 或运行脚本 |
| **pip** | `pip <version> <pip args...>` | 以指定版本运行 pip（自动清华镜像 + PyTorch CUDA 源） |
| **pyins** | `pyins <version> [--force]` | 下载并安装嵌入式 Python 到当前盘根目录 `<盘>:\py<version>` |
| **pyunin** | `pyunin <version> [--force]` / `pyunin --all` | 卸载 pyins 安装的 Python（删目录 + 清 config.json 记录） |
| **crenv** | `crenv <version> <env_name>` | 安装 virtualenv 并创建虚拟环境 |
| **install** | `install <version>` | 通过 pip 安装 `requirements.txt` 依赖 |
| **uninstall** | `uninstall <version>` | 通过 pip 卸载 `requirements.txt` 依赖 |
| **pip-clear** | `pip-clear [version]` | 清除非核心 pip 包（保留 pip/setuptools/wheel 等） |
| **dels** | `dels <path> [path...]` | 强制删除文件/目录（绕过只读/系统属性） |

## 快速开始

### 1. 下载安装脚本

安装脚本不入 Release，直接下载：

**[install.ps1](https://raw.githubusercontent.com/ycm50/pytools/master/install.ps1)**（[Releases 页](https://github.com/ycm50/pytools/releases)只放二进制发布包）

```powershell
iwr https://raw.githubusercontent.com/ycm50/pytools/master/install.ps1 -OutFile install.ps1
```

### 2. 安装（一行命令）

**联网安装最新发布版**（推荐，脚本自动去 Releases 取最新 tag 的 zip）：

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1
```

**离线安装**（已手动下载 Release 里的 zip）：

```powershell
powershell -ExecutionPolicy Bypass -File .\install.ps1 -ZipPath .\pytools-v1.0.0-windows-x64.zip
```

安装脚本会：

- 从 GitHub Releases **最新正式 tag** 下载发布包（zip），或者使用 `-ZipPath` 指定的本地包；
- 把包内全部文件（9 个 exe + `README.md` 说明书 + `uninstall.bat` 卸载脚本）安装到**当前盘根目录下的 `pytools` 目录**
  （例如当前目录在 `D:\work`，则装到 `D:\pytools`）；
- 升级时**保留原有 `config.json`**（已装的 Python 版本记录不丢）；
- 把该目录**插入系统 PATH 的最顶端**（优先级最高，自动去重；需要管理员权限，未提权时自动请求 UAC），
  并清理旧版本遗留在**用户 PATH** 里的同名项。

可选参数：

```powershell
.\install.ps1 -Version v1.1.0            # 安装指定 tag（默认 latest）
.\install.ps1 -Proxy http://127.0.0.1:10808   # 指定下载代理（默认读 HTTPS_PROXY）
.\install.ps1 -DownloadUrl <zip 地址>     # 直接用某个下载地址 / 镜像
.\install.ps1 -Drive D                   # 指定盘符
.\install.ps1 -InstallDir E:\tools       # 直接指定安装目录
.\install.ps1 -NoPath                    # 只安装文件，不改 PATH
```

新开一个终端后即可使用 `py`、`pyins` 等命令。

### 3. 安装 Python

在**目标盘**任意目录执行 `pyins`，它会自动识别当前工作目录所在盘：

```cmd
cd /d D:\work
pyins 3.12.1        # 安装到 D:\py3.12.1，并让当前目录的 python 处于可交互状态
```

### 4. 卸载 Python

```cmd
pyunin 3.12.1           # 交互确认后删除指定版本
pyunin 3.12.1 --force   # 不询问直接删除
pyunin --all --force    # 卸载全部由 pyins 安装的版本
pyunin --all            # 卸载全部，删前确认
```

删除的是 `pyins` 装出来的 `<盘>:\py<版本>` 目录，同时把 `config.json` 里对应版本的记录移除。
删除**不进回收站**；目录名不是 `py<版本>` 时（配置被改坏的情况）会拒绝删除，避免误删。

`--all` 只扫描**本 pytools 安装目录所在盘**的根目录，不会碰其他盘的 `py*` 目录。

### 5. 完整卸载 pytools

安装目录里的 `uninstall.bat` 会一次做完三件事：

```cmd
uninstall.bat       :: 交互确认
uninstall.bat /y    :: 不询问
```

1. 调用 `pyunin --all --force` 卸载全部由 pyins 安装的 Python；
2. 从**系统 PATH** 和**用户 PATH** 中移除安装目录（并广播环境变量变更）；
3. 删除安装目录自身（延迟删除，避开脚本占用）。

删除同样**不进回收站**。

### 6. 运行 Python

```cmd
py 3.12.1                   # 启动 REPL
py 3.12.1 script.py         # 运行脚本
py 3.12.1 -c "print('hi')"  # 内联代码
```

### 7. 管理包

```cmd
pip 3.12.1 install numpy torch   # 自动使用清华镜像 + PyTorch CUDA 源
pip 3.12.1 list
install 3.12.1                   # 从 requirements.txt 安装
pip-clear 3.12.1                 # 清除非核心包
```

### 8. 虚拟环境

```cmd
crenv 3.12.1 myenv
myenv\Scripts\activate
```

## 盘符规则

盘符按以下优先级确定（第一个可用的生效）：

1. 环境变量 `PYTOOLS_DRIVE`，例如 `set PYTOOLS_DRIVE=D`；
2. **当前工作目录所在盘**（自动识别，默认行为）；
3. 与二进制同目录的 `config.json` 中记录的 `drive`；
4. 编译期默认盘符（CMake 变量 `PYTOOLS_DRIVE_LETTER`，默认 `a`）。

因此 `pyins` 装到哪里，取决于你在哪个盘执行它。

## config.json

`pyins` 安装完成后，会在**与二进制同目录**的 `config.json` 中登记 Python 位置，
`py`、`pip`、`crenv`、`install`、`uninstall`、`pip-clear`、`pyunin` 等工具都从这里查找解释器：

```json
{
  "drive": "D:",
  "install_root": "D:\\",
  "pythons": {
    "3.12.1": "D:\\py3.12.1\\python.exe"
  }
}
```

某个版本在配置中不存在时，回退为 `<盘>:\py<version>\python.exe`。
每次 `pyins` 成功都会合并写入，不会丢失其他版本的记录。

## 从源码构建

依赖：CMake ≥ 3.15、C++17 编译器（MinGW GCC / MSVC）。

```cmd
cmake -B build
cmake --build build
```

指定默认盘符（可选，仅在无法自动识别时兜底）：

```cmd
cmake -B build -DPYTOOLS_DRIVE_LETTER=D
cmake --build build
```

编译产物在 `build/bin/`。

## 发布（GitHub Actions）

`.github/workflows/release.yml` 为**手动触发**流程，唯一输入是**tag 号**：

1. 矩阵构建：MSYS2 UCRT64 与 MINGW64 两套 MinGW 工具链各自编译；
2. 任一构建失败即整体失败，**不会创建任何 tag**；
3. 全部构建成功后才创建并推送 `tag`，随后发布 Release 并附带 `pytools-<tag>-windows-x64.zip`（标准 zip 格式，内含 9 个二进制 + `README.md` 说明书 + `uninstall.bat`；`install.ps1` 不入 Release）。

触发方式：Actions → Release → Run workflow → 填入 tag 号（例如 `v1.0.0`）。

## 项目结构

```
pytools/
├── CMakeLists.txt            # 顶层 CMake 构建
├── install.ps1               # 安装脚本（下载最新 Release、装到 <盘>:\pytools、插到系统 PATH 顶端）
├── uninstall.bat             # 卸载脚本（pyunin --all + 移除 PATH + 删除安装目录）
├── .github/workflows/
│   └── release.yml           # 手动触发：构建矩阵 → 建 tag → Release
├── src/
│   ├── CMakeLists.txt        # 源码构建（静态库 + 9 个 exe，并复制脚本/说明书到 bin）
│   ├── common/
│   │   ├── utils.h/.cpp      # 盘符识别、config.json、CreateProcess/下载/ZIP解压/强制删除
│   │   └── json.h/.cpp       # 极简 JSON 读写（config.json 用）
│   ├── py/main.cpp           # py 工具
│   ├── pip/main.cpp          # pip 工具
│   ├── pyins/main.cpp        # Python 安装工具
│   ├── pyunin/main.cpp       # Python 卸载工具
│   ├── crenv/main.cpp        # 虚拟环境创建工具
│   ├── install_tool/main.cpp # 依赖安装工具
│   ├── uninstall_tool/main.cpp # 依赖卸载工具
│   ├── pip-clear/main.cpp    # pip 清理工具
│   └── dels/main.cpp         # 强制删除工具
└── build/                    # 编译输出（已加入 .gitignore）
```
