import sys
import os
import zipfile
import glob
import subprocess
import winreg
from pathlib import Path
def get_download_folder():
    sub_key = r'SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders'
    downloads_guid = '{374DE290-123F-4565-9164-39C4925E467B}'
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, sub_key) as key:
        location = winreg.QueryValueEx(key, downloads_guid)[0]
    return location.replace('%%USERPROFILE%%', str(Path.home()))

version = sys.argv[1] if len(sys.argv) > 1 else '3.11.5'

try:
    os.chdir(get_download_folder())
except Exception as e:
    print(f'切换目录失败: {e}')
    sys.exit(1)

# 下载Python嵌入版
py_url = f'https://www.python.org/ftp/python/{version}/python-{version}-embed-amd64.zip'
try:
    os.system(f'curl -o py{version}.zip {py_url}')
except Exception as e:
    print(f'下载Python失败: {e}')
    sys.exit(1)

# 下载get-pip
pip_url = 'https://bootstrap.pypa.io/get-pip.py'
try:
    os.system(f'curl -o get-pip.py {pip_url}')
except Exception as e:
    print(f'下载get-pip失败: {e}')
    sys.exit(1)

# 创建目标目录
target_dir = f'b:/py{version}'
os.makedirs(target_dir, exist_ok=True)

# 解压文件
try:
    with zipfile.ZipFile(f'py{version}.zip') as z:
        z.extractall(target_dir)
except Exception as e:
    print(f'解压失败: {e}')
    sys.exit(1)

# 修改._pth文件
for pth_file in glob.glob(os.path.join(target_dir, '*._pth')):
    try:
        with open(pth_file, 'a') as f:
            f.write('\nimport site')
    except Exception as e:
        print(f'修改pth文件失败: {e}')

# 安装pip
try:
    subprocess.run(
        [os.path.join(target_dir, 'python.exe'), 'get-pip.py'],
        stderr=subprocess.DEVNULL,
        check=True
    )
except Exception as e:
    print(f'安装pip失败: {e}')

# 清理文件
try:
    os.remove('get-pip.py')
    os.remove(f'py{version}.zip')
except Exception as e:
    print(f'清理文件失败: {e}')