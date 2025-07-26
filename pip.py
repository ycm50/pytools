import subprocess
import os
import sys

pipver = ''
argvs = sys.argv
if len(argvs) < 2:
    exit()
pipver = argvs[1]

python_exe = f'a:/py{pipver}/python.exe'
if not os.path.exists(python_exe):
    print(f"Error: Python interpreter not found at {python_exe}")
    sys.exit(1)

pip_command = [python_exe, '-m', 'pip']
command = pip_command + argvs[2:] + [
    '--index-url', 'https://pypi.tuna.tsinghua.edu.cn/simple',
    '--trusted-host', 'pypi.tuna.tsinghua.edu.cn',
    '--extra-index-url', 'https://download.pytorch.org/whl/cu124'
] if (len(argvs) > 2 and argvs[2] == 'install') else pip_command + argvs[2:]

try:
    subprocess.run(command, check=True)
except subprocess.CalledProcessError as e:
    print(f"pip command failed: {e}")
    sys.exit(1)
