@echo off
set port=%1

if "%port%"=="" (
    set http_proxy=
    set https_proxy=
) else (
    set http_proxy=127.0.0.1:%port%
    set https_proxy=127.0.0.1:%port%
)

REM 输出当前环境变量值以确认
echo http_proxy: %http_proxy%
echo https_proxy: %https_proxy%

pause