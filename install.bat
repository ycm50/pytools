@echo off
setlocal enabledelayedexpansion

REM 如果提供了其他.txt文件名参数，则使用该文件，否则使用requirements.txt
if "%~1" neq "" (
    set req_file=%~1
) else (
    set req_file=requirements.txt
)

REM 检查文件是否存在
if exist "!req_file!" (
    echo 正在安装依赖...
    pip install -r "!req_file!"
    if %errorlevel% equ 0 (
        echo 依赖安装成功！
    ) else (
        echo 依赖安装失败，错误代码：%errorlevel%
    )
) else (
    echo 文件 !req_file! 不存在！
)

endlocal