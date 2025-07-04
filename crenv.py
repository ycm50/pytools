import sys
import subprocess

# 检查参数数量是否足够
if len(sys.argv) < 3:
    print('参数不足，需要提供 ver 和 env_name 参数')
    sys.exit(1)

# 获取参数
ver = sys.argv[1]
env_name = sys.argv[2]

# 定义 pip 命令
pip_command = f"b:/py{ver}/Scripts/pip.exe install virtualenv".split()
# 执行 pip 命令
try:
    subprocess.run(pip_command, check=True)
except subprocess.CalledProcessError as e:
    print(f"pip 命令执行失败: {e}")
    sys.exit(1)

# 定义 virtualenv 命令
virtualenv_command = f"b:/py{ver}/python.exe -m virtualenv {env_name}".split()
# 执行 virtualenv 命令
try:
    subprocess.run(virtualenv_command, check=True)
except subprocess.CalledProcessError as e:
    print(f"virtualenv 命令执行失败: {e}")
    try:
        virtualenv_command = f"b:/py{ver}/python.exe -m  virtualenv --always-copy {env_name}".split()
        subprocess.run(virtualenv_command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"第二次尝试 virtualenv 命令执行失败: {e}")
        sys.exit(2)
        
