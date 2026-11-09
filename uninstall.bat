@echo off
setlocal EnableExtensions
chcp 65001 >nul 2>&1

rem ============================================================
rem  pytools 卸载脚本
rem  1) 用 pyunin 卸载所有由 pyins 安装的 Python
rem  2) 从系统 PATH / 用户 PATH 中移除本目录
rem  3) 删除本目录（延迟删除，脚本自身也在这里）
rem  删除不进回收站
rem  用法: uninstall.bat [/y]
rem ============================================================

set "SELFDIR=%~dp0"
if "%SELFDIR:~-1%"=="\" set "SELFDIR=%SELFDIR:~0,-1%"
set "PYUNIN=%SELFDIR%\pyunin.exe"

echo ============================================
echo  pytools 卸载
echo  安装目录: %SELFDIR%
echo ============================================
echo.

if /i "%~1"=="/y" goto run
if /i "%~1"=="-y" goto run
if /i "%~1"=="--yes" goto run
set "ANS="
set /p "ANS=确认卸载全部由 pyins 安装的 Python 并删除本目录？[y/N] "
if /i not "%ANS%"=="y" (
    echo 已取消
    exit /b 1
)

:run
echo [1/3] 卸载全部由 pyins 安装的 Python...
if exist "%PYUNIN%" (
    "%PYUNIN%" --all --force
    if errorlevel 1 echo   警告: 部分 Python 卸载失败，继续后续步骤
) else (
    echo   警告: 未找到 pyunin.exe，跳过 Python 卸载
)

echo.
echo [2/3] 从系统 PATH / 用户 PATH 中移除本目录...
set "PS1=%TEMP%\pytools_uninstall_path.ps1"
> "%PS1%" echo param([switch]$Elevated, [string]$Dir)
>> "%PS1%" echo $dir = if ($Dir) { $Dir } else { $env:PT_UNINSTALL_DIR }
>> "%PS1%" echo if (-not $dir) { exit 0 }
>> "%PS1%" echo $dir = $dir.TrimEnd([char]92)
>> "%PS1%" echo $isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
>> "%PS1%" echo $u = [Environment]::GetEnvironmentVariable('Path', 'User')
>> "%PS1%" echo if ($u) { $parts = @($u -split ';' ^| Where-Object { $_ -ne '' -and $_.TrimEnd([char]92) -ne $dir }); $new = $parts -join ';'; if ($new -ne $u) { [Environment]::SetEnvironmentVariable('Path', $new, 'User'); Write-Host '  已从用户 PATH 移除' } }
>> "%PS1%" echo if ($isAdmin) { $m = [Environment]::GetEnvironmentVariable('Path', 'Machine'); if ($m) { $parts = @($m -split ';' ^| Where-Object { $_ -ne '' -and $_.TrimEnd([char]92) -ne $dir }); $new = $parts -join ';'; if ($new -ne $m) { [Environment]::SetEnvironmentVariable('Path', $new, 'Machine'); Write-Host '  已从系统 PATH 移除' } else { Write-Host '  系统 PATH 中没有该目录' } } } else { Write-Host '  需要管理员权限清理系统 PATH，正在请求提权...'; Start-Process powershell -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',$PSCommandPath,'-Elevated','-Dir',$dir) -Verb RunAs -Wait }
>> "%PS1%" echo $sig = '[DllImport("user32.dll", CharSet=CharSet.Auto)] public static extern System.IntPtr SendMessageTimeout(System.IntPtr hWnd, uint Msg, System.UIntPtr wParam, string lParam, uint fuFlags, uint uTimeout, out System.UIntPtr lpdwResult);'
>> "%PS1%" echo $t = Add-Type -MemberDefinition $sig -Name U -Namespace W -PassThru
>> "%PS1%" echo $R = [System.UIntPtr]::Zero
>> "%PS1%" echo [void]$t::SendMessageTimeout([System.IntPtr]0xffff, 0x1A, [System.UIntPtr]::Zero, 'Environment', 2, 5000, [ref]$R)
set "PT_UNINSTALL_DIR=%SELFDIR%"
powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%"
del /f /q "%PS1%" >nul 2>&1

echo.
echo [3/3] 删除安装目录...
cd /d "%TEMP%"
if not exist "%SELFDIR%" (
    echo   目录不存在，跳过
    goto done
)
start "" /min cmd /c "ping -n 4 127.0.0.1 >nul & rmdir /s /q ""%SELFDIR%"""
echo   已安排删除: %SELFDIR%
echo   若文件被占用，稍后手动删除该目录即可

:done
echo.
echo 卸载完成。
exit /b 0
