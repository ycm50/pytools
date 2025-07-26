import os
import sys
ver=""
if len(sys.argv) > 2:
    print('Usage: install / install python.ver')
    print('Example: install 3.10.6')
    print('Example: install')
    sys.exit(1)
elif len(sys.argv) == 2:
    ver = sys.argv[1]
try:
    os.system(f"pip {ver} uninstall -r requirements.txt -y")
except subprocess.CalledProcessError as e:
    print(f"安装失败: {e}")
