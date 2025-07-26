from nt import system
import os
import sys
import zipfile
import subprocess
import glob
import ctypes
import ctypes.wintypes as wintypes

# 修正后的 GUID 类定义
class GUID(ctypes.Structure):
    _fields_ = [
        ("Data1", ctypes.c_ulong),
        ("Data2", ctypes.c_ushort),
        ("Data3", ctypes.c_ushort),
        ("Data4", ctypes.c_ubyte * 8)
    ]

    def __init__(self, guid_string):
        # 解析 GUID 字符串，如 "{3D644C9B-1FB8-4F30-9B45-F670235F79C0}"
        parts = guid_string.strip("{}").split('-')
        self.Data1 = int(parts[0], 16)
        self.Data2 = int(parts[1], 16)
        self.Data3 = int(parts[2], 16)
        
        # 将生成器转换为元组再合并
        data4_part1 = tuple(int(parts[3][i:i+2], 16) for i in (0, 2))
        data4_part2 = tuple(int(parts[4][i:i+2], 16) for i in range(0, 12, 2))
        self.Data4 = (ctypes.c_ubyte * 8)(*data4_part1 + data4_part2)

def get_downloads_directory():
    # 定义 FOLDERID_Downloads 的 GUID
    FOLDERID_Downloads = GUID('{3D644C9B-1FB8-4F30-9B45-F670235F79C0}')

    SHGetKnownFolderPath = ctypes.windll.shell32.SHGetKnownFolderPath
    SHGetKnownFolderPath.argtypes = [
        ctypes.POINTER(GUID),  # 修改为自定义的 GUID 类型
        wintypes.DWORD,
        wintypes.HANDLE,
        ctypes.POINTER(ctypes.c_wchar_p)
    ]

    path_ptr = ctypes.c_wchar_p()
    result = SHGetKnownFolderPath(ctypes.byref(FOLDERID_Downloads), 0, None, ctypes.byref(path_ptr))
    if result == 0:
        return path_ptr.value
    else:
        raise RuntimeError(f"无法获取下载目录，错误代码: {result}")

# 主程序逻辑
# =============================
ver = sys.argv[1] if len(sys.argv) > 1 else '3.11.5'
url = f'https://www.python.org/ftp/python/{ver}/python-{ver}-embed-amd64.zip'

downloads_path = get_downloads_directory()
print(downloads_path)
target_dir = f'a:\\py{ver}'

os.chdir(downloads_path)
filename = f'py{ver}.zip'

print(f'正在下载 Python {ver} 压缩包...')
subprocess.run(['curl.exe', '-L', url, '-o', filename], check=True)

print(f'正在解压到 {target_dir}...')
os.makedirs(target_dir, exist_ok=True)
with zipfile.ZipFile(filename, 'r') as zip_ref:
    zip_ref.extractall(target_dir)
os.remove(filename)

# 修改 ._pth 文件
pth_files = glob.glob(os.path.join(target_dir, 'python*._pth'))
if pth_files:
    pth_file = pth_files[0]
    print(f'正在修改 {os.path.basename(pth_file)}...')
    with open(pth_file, 'a') as f:
        f.write('\nimport site')

# 下载 get-pip.py
ver2 = ver.split('.')
ver3=ver2[0]+'.'+ver2[1]
pip_url = 'https://bootstrap.pypa.io/get-pip.py'if int(ver2[1]) >= 9 else f'https://bootstrap.pypa.io/pip/{ver3}/get-pip.py'
pip_file = os.path.join(target_dir, 'get-pip.py')
subprocess.run(['curl.exe', '-L', pip_url, '-o', pip_file], check=True)


# 安装 pip
python_exe = os.path.join(target_dir, 'python.exe')
try:
    subprocess.run([python_exe, pip_file], check=True)
    print('pip 安装完成')
except Exception as e:
    print(f'安装 pip 失败: {e}')
os.remove(pip_file)
print('Python 压缩版安装完成（便携版）')