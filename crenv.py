import sys
import subprocess

# 检查参数数量是否足够
ver = ''
if len(sys.argv) < 3:
    print('参数不足，需要提供 ver 和 env_name 参数')
    sys.exit(1)

# 获取参数
ver = sys.argv[1] if '.' in sys.argv[1] else ''
env_name = sys.argv[2]if'.' in sys.argv[1] else sys.argv[1]

print(f"正在为 Python {ver} 创建虚拟环境 {env_name}")

# 定义 pip 命令
pip_command = [ "pip", f"{ver}", "install", "virtualenv"]
# 执行 pip 命令
try:
    subprocess.run(pip_command, check=True)
    print("virtualenv 安装成功")
except subprocess.CalledProcessError as e:
    print(f"pip 命令执行失败: {e}")
    sys.exit(1)

# 定义 virtualenv 命令
virtualenv_command = f"a:/py{ver}/scripts/virtualenv.exe {env_name}".split()
# 执行 virtualenv 命令
try:
    subprocess.run(virtualenv_command, check=True)
    print(f"虚拟环境 {env_name} 创建成功")
except subprocess.CalledProcessError as e:
    print(f"virtualenv 命令执行失败: {e}")

        
