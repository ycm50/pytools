import sys
import subprocess
import os

def main():
    if len(sys.argv) < 2:
        print('Usage: py.exe <version> [command]')
        print('Example: py.exe 3.2.3')
        print('Example: py.exe 3.2.3 -c "print(\'Hello\')"')
        sys.exit(1)

    version = sys.argv[1]
    python_path = f"a:/py{version}/python.exe"

    if not os.path.exists(python_path):
        print(f"Error: Python {version} interpreter not found at {python_path}")
        sys.exit(1)

    if len(sys.argv) == 2:
        print(f"=== Starting Python {version} REPL ===")
        subprocess.run([python_path], check=True)
    else:
        # 直接传递剩余参数，不拼接字符串
        try:
            subprocess.run([python_path] + sys.argv[2:], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Command failed: {e}")
            sys.exit(1)

if __name__ == "__main__":
    main()