import os
import shutil
from pathlib import Path

def safe_delete(target_path=None):
    # 获取工作目录绝对路径
    workspace = Path.cwd().resolve()
    
    # 确定要删除的路径
    if not target_path:
        delete_root = workspace
        preserve_root = True
    else:
        # 增强路径解析逻辑
        try:
            target = Path(target_path).resolve(strict=True)
        except FileNotFoundError:
            raise FileNotFoundError(f"路径解析失败: {target_path}")

        # Windows系统文件特殊处理
        if target.is_file():
            try:
                import win32api
                win32api.SetFileAttributes(str(target), win32api.FILE_ATTRIBUTE_NORMAL)
            except:
                os.chmod(target, 0o777)
            # Windows系统文件属性强制清除
            try:
                import win32con
                win32api.SetFileAttributes(str(target), win32con.FILE_ATTRIBUTE_NORMAL)
            except Exception:
                pass
        delete_root = target
        preserve_root = False

    # 执行删除操作
    try:
        if delete_root.is_file():
            os.remove(delete_root)
        elif delete_root.is_dir():
            for item in delete_root.glob('*'):
                if item.is_dir():
                    shutil.rmtree(item)
                else:
                    os.remove(item)
            
            # 保留目录本身当需要时
            if preserve_root:
                print(f"已清理目录内容：{delete_root}")
            else:
                shutil.rmtree(delete_root)
                print(f"已删除目录：{delete_root}")
    except Exception as e:
        print(f"删除错误：{str(e)}")

if __name__ == '__main__':
    import sys
    
    if len(sys.argv) > 1:
        # 路径规范化处理
        args = [os.path.normpath(arg) for arg in sys.argv[1:]]
        for arg in args:
            safe_delete(arg)
    else:
        safe_delete()