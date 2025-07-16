import sys
import os
import subprocess
import ctypes
from ctypes import wintypes
ver = sys.argv[1] if len(sys.argv) > 1 else '3.11.5'

def get_downloads_directory():
    SHGetFolderPath = ctypes.windll.shell32.SHGetFolderPathW
    SHGetFolderPath.argtypes = [wintypes.HWND, ctypes.c_int, wintypes.HANDLE, wintypes.DWORD, wintypes.LPCWSTR]
    CSIDL_DOWNLOADS = 0x0019
    MAX_PATH = 260
    path_buf = ctypes.create_unicode_buffer(MAX_PATH)
    result = SHGetFolderPath(None, CSIDL_DOWNLOADS, None, 0, path_buf)
    if result == 0:
        return path_buf.value
    else:
        raise RuntimeError(f"无法获取下载目录，错误代码: {result}")

os.chdir(get_downloads_directory())


py_url = f'https://www.python.org/ftp/python/{ver}/python-{ver}-amd64.exe'
try:
    os.system(f'curl.exe  -o py{ver}.exe {py_url}')
except Exception as e:
    print(f'下载Python失败: {e}')
    sys.exit(1)
os.system(f'py{ver}.exe /quiet TargetDir=b:\\py{ver} InstallAllUsers=1 PrependPath=0')
# 清理文件
try:
    os.remove(f'py{ver}.exe')
except Exception as e:
    print(f'清理文件失败: {e}')
