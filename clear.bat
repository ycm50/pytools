@echo off
echo 正在关闭系统代理（WinHTTP）...
netsh winhttp reset proxy
echo 代理已关闭。
@echo off
echo 正在清理临时文件...
RD /S /Q "%TEMP%" 2>nul  # 强制删除用户临时文件夹
MKDIR "%TEMP%" 2>nul     # 重建临时文件夹
RD /S /Q "C:\Windows\Temp" 2>nul  # 清理系统临时文件
echo 清理完成！
