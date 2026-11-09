# 测试列表（全量测试用）

> 规则：只有在我说「全局测试」时才按本列表整体执行；平时只测新增/修改部分。
> 本列表随功能变更同步更新。

## 自动化覆盖（脚本: `使用tmpscripts里的运行器，不入库`）

| 编号 | 测试项 | 预期结果 |
|------|--------|----------|
| T01 | CMake 配置 + 编译 9 个二进制 | 编译成功，`build/bin` 下 9 个 exe 齐全 |
| T02 | `pyins` 参数校验 | `--help` 退出 0；无参数退出 1；`3.12`（非法版本）退出 1 |
| T03 | 盘符解析优先级 | cwd 在 A 盘→`A:`；切到 C 盘→`C:`；`PYTOOLS_DRIVE=D`→`D:`；config 记录的盘不覆盖 cwd |
| T04 | `py` 未安装版本的报错 | 提示正确盘符路径、退出码 1 |
| T05 | `pyins 3.12.1` 真实安装到当前盘根目录 | 安装到 `A:\py3.12.1`，`._pth` 的 `import site` 生效，pip 可用 |
| T06 | `config.json` 写入与合并 | 与 exe 同目录生成合法 JSON；重复 `pyins` 不丢已有版本记录，已安装则跳过 |
| T07 | `py` 运行能力 | `-c` 内联、脚本文件、`--version`、REPL 管道输入均正常 |
| T08 | `pip` 工具 | `--version`、`list` 正常；`install` 走清华镜像成功 |
| T09 | `install` 工具 | 按 `requirements.txt` 安装依赖成功 |
| T10 | `uninstall` 工具 | 按 `requirements.txt` 卸载依赖成功 |
| T11 | `crenv` 工具 | 创建虚拟环境，`Scripts\python.exe` 可执行 |
| T12 | `pip-clear` 工具 | 清除非核心包，保留 pip/setuptools/wheel |
| T13 | `dels` 工具 | 删除只读文件与目录成功 |
| T14 | `install.ps1` | `-NoPath` 只复制不写任何 PATH；正常模式把目录插入**系统 PATH 最顶端**并去重，同时清理用户 PATH 同名项；`-Drive` 生效；无效盘符报错；未提权时自动请求 UAC |
| T14b | `install.ps1` 从最新 Release 安装 | 走代理下载最新 tag 的 zip 包并解压安装，8 个 exe 齐全 |
| T14c | `install.ps1 -ZipPath` 离线安装 | 本地 zip 安装成功，包含 README.md 说明书 |
| T14d | 升级保留 config.json | 重装后已有 Python 版本记录不丢 |
| T14e | `-Version` 不存在时 | 报错退出，不产生半安装 |
| T16 | `pyunin` 卸载 | 删除 `<盘>:\py<ver>` 目录并移除 config.json 记录；目录不存在时只清记录；路径不是 `py<ver>` 时拒绝删除；删除不进回收站 |
| T16b | `pyunin --all` | 卸载配置中全部版本 + 本盘根目录扫描到的 `py<版本>`；**不扫描其他盘**；无可卸载项时正常退出 |
| T18 | `uninstall.bat` | `/y` 免确认；调用 pyunin --all 卸载 Python、从系统 PATH 与用户 PATH 移除安装目录、删除安装目录自身；删除不进回收站 |
| T17 | 临时文件清理 | `pyins` 与 `install.ps1` 的下载包都落在 `%TEMP%`（动态读 `TEMP`/`TMP`，可指向任意盘）；成功、下载失败、解压失败、版本不存在四种情况都不留残留 |
| T15 | GitHub Actions workflow | YAML 合法；`workflow_dispatch` 带 `tag` 输入；`release` 依赖 `build`；建 tag 在所有构建之后；产物为 zip 且含 README 说明书与 uninstall.bat（install.ps1 不入 Release） |

## 手工/联网项

| 编号 | 测试项 | 说明 |
|------|--------|------|
| M01 | Actions 手动触发全流程 | 需要 GitHub 上真实运行：构建矩阵 → 建 tag → Release |
| M02 | 新终端 PATH 生效 | 安装后新开终端能直接使用 `py`/`pyins` |

