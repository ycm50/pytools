#!/usr/bin/env python
import os
import sys
import re
import shutil
import zipfile
import glob
from pathlib import Path
import subprocess

# 获取系统下载文件夹路径
def get_download_folder():
    import winreg
    sub_key = r'SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders'
    downloads_guid = '{374DE290-123F-4565-9164-39C4925E467B}'
    with winreg.OpenKey(winreg.HKEY_CURRENT_USER, sub_key) as key:
        location = winreg.QueryValueEx(key, downloads_guid)[0]
    return location.replace('%%USERPROFILE%%', str(Path.home()))

# 替换文件内容
def replace_in_file(file_path, disk_name, download_folder):
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            if str(file_path).endswith('build.py'):
                content=""
            else:
                content = f.read()
                content = re.sub(r'b:\\', rf'{disk_name}:/', content)
                content = re.sub(r'b:/', rf'{disk_name}:/', content)
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(content)
    except Exception as e:
        print(f"处理文件 {file_path} 时出错: {e}")

# 编译文件
def compile_files(disk_name):
    current_dir = Path.cwd()
    # 编译py文件
    for py_file in current_dir.glob('*.py'):
        if py_file.name == 'build.py':
            continue
        exe_file = current_dir / f'{py_file.stem}.exe'
        try:
            # 使用子文件夹内的环境编译
            env_path = current_dir / 'py' / 'Scripts'
            env = os.environ.copy()
            env['PATH'] = str(env_path) + os.pathsep + env['PATH']
            subprocess.run([str(env_path / 'python.exe'), '-m', 'PyInstaller', '--onefile', '--distpath', str(current_dir), str(py_file)], check=True, env=env)
            # 清理PyInstaller生成的文件
            if (current_dir / 'build').exists():
                shutil.rmtree(current_dir / 'build')
            spec_file = current_dir / f'{py_file.stem}.spec'
            if spec_file.exists():
                spec_file.unlink()
        except subprocess.CalledProcessError as e:
            print(f"编译 {py_file} 时出错: {e}")
        except Exception as e:
            print(f"处理 {py_file} 时出错: {e}")

# 清理编译过程产生的文件
def clean_build_files():
    current_dir = Path.cwd()
    for item in current_dir.iterdir():
        if item.is_file() and item.suffix.lower() in ['.o', '.obj', '.d', '.a', '.lib', '.exp', '.pdb']:
            item.unlink()

if __name__ == '__main__':
    with zipfile.ZipFile(f'py.zip') as z:
        z.extractall("./")
    if len(sys.argv) != 2:
        print("用法: python build.py <磁盘名称>")
        sys.exit(1)
    disk_name = sys.argv[1].lower()
    if len(disk_name) != 1 or not disk_name.isalpha():
        print("磁盘名称必须是单个字母")
        sys.exit(1)
    download_folder = get_download_folder()
    current_dir = Path.cwd()
    # 替换所有非exe文件内容
    for file_path in current_dir.glob('*'):
        if file_path.is_file() and file_path.suffix.lower() != '.exe':
            replace_in_file(file_path, disk_name, download_folder)
    # 编译文件
    compile_files(disk_name)
    # 清理编译过程产生的文件
    clean_build_files()
    print('编译完成，请加入环境变量，此时可以删除py文件夹')