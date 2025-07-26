import os
from shlex import join
import sys

def uninstall_all_pip_packages(ver):
    # 基础工具包关键字列表（不区分大小写）
    exclude_keywords = ['pyinstaller', 'torch', 'virtualenv', 'pip', 'setuptools', 'wheel','pyinstaller']

    # 获取pip安装的所有包列表
    try:
        command = ['pip', ver, 'freeze'] if ver else ['pip', 'freeze']
        with os.popen(join(command)) as pipe:
            installed_pkgs = pipe.read().splitlines()
        
        # 提取包名（忽略版本号），并过滤掉基础工具包
        filtered_pkgs = []
        for pkg in installed_pkgs:
            package_name = pkg.split('==')[0].strip()  # 取包名部分
            if not any(keyword.lower() in package_name.lower() for keyword in exclude_keywords):
                filtered_pkgs.append(package_name)

        if not filtered_pkgs:
            print("没有可卸载的包。")
            return

        print("即将卸载以下包：")
        for name in filtered_pkgs:
            print(f" - {name}")

        # 执行卸载命令
        uninstall_cmd = ['pip', ver, 'uninstall']
        uninstall_cmd += filtered_pkgs + ['-y', '--no-input']
        os.system(join(uninstall_cmd))

    except Exception as e:
        print("操作失败:", str(e))
        return

if __name__ == "__main__":
    ver = ''
    if len(sys.argv) > 2:
        print('用法: pip-clear [/ pip-clear python.ver]')
        print('例: pip-clear 3.10.6')
        print('例: pip-clear')
        sys.exit(1)
    elif len(sys.argv) == 2:
        ver = sys.argv[1]
    
    uninstall_all_pip_packages(ver)