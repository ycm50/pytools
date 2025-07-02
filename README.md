# pytools

本仓库为 Python 环境及工具集合，仅主目录为核心，子目录为环境配置可忽略。

## 主要文件说明与用法

- **build.exe**  
  用法：`build d`  
  说明：d 为盘符（不带冒号），用于编译以下 exe 工具，并在选定盘符新建 py 安装文件夹。

- **pyins.exe**  
  用法：`pyins py版本号`  
  说明：安装指定 Python 版本到通过 build.exe 选定的驱动器。

- **crenv.exe**  
  用法：`crenv py版本号 虚拟环境名`  
  说明：创建指定 Python 版本的虚拟环境。

- **pip.exe**  
  用法：`pip py版本号 pip命令`  
  说明：用对应 Python 版本执行 pip 命令。例如：`pip 312 install numpy`

- **py.exe**  
  用法：`py py版本号 命令`  
  说明：用指定 Python 版本执行命令。例如：`py 310 script.py`

- **env.exe**  
  用法：`env 虚拟环境名`  
  说明：进入已创建的虚拟环境。

- **ps.bat**  
  功能：在 cmd 下启动 PowerShell。

- **install.bat / install.ps1**  
  说明：Windows 下的安装辅助脚本，可选用。

- **env.cpp**  
  说明：环境相关工具的 C++ 源码。

## 使用建议

1. 环境搭建请首先使用 `build.exe`，如 `build d`，d 为目标盘符。
2. 之后用 `pyins.exe` 安装所需 Python 版本，`crenv.exe` 建立虚拟环境。
3. 其余工具可根据上述说明使用。

## 其他

- 子目录为不同环境补充内容，普通用户无需关注。
- 如需查看更多文件，请前往 [GitHub 主目录](https://github.com/ycm50/pytools/tree/master)。

---

如有问题请提交 Issue，欢迎 PR！
