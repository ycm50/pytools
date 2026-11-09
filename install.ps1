<#
.SYNOPSIS
    pytools 安装脚本：从 GitHub Releases 最新 tag（或指定版本）下载 zip 包，
    安装到指定盘根目录下的 pytools 目录，并插入系统 PATH 的最顶端。

.DESCRIPTION
    默认从仓库 ycm50/pytools 的「最新正式 Release」下载发布包；
    安装目录默认取「当前工作目录所在盘」根目录下的 pytools，例如当前目录在 D:\work，
    则安装到 D:\pytools。安装完成后把该目录插到「系统 PATH」的最顶端（需要管理员权限，
    未提权时会自动请求 UAC 提权完成这一步），同时清理旧版本可能留在用户 PATH 里的同名项。

.PARAMETER Version
    要安装的 tag，默认 latest（最新正式 Release）。

.PARAMETER Repo
    仓库，默认 ycm50/pytools。

.PARAMETER DownloadUrl
    直接指定发布包下载地址（镜像/内网/固定链接时使用），优先于 -Version。

.PARAMETER ZipPath
    使用本地已有的发布包（离线安装），不再联网下载。

.PARAMETER Proxy
    下载使用的代理，例如 http://127.0.0.1:10808；默认读取 HTTPS_PROXY / HTTP_PROXY。

.PARAMETER Drive
    指定盘符，如 D 或 D:，默认取当前工作目录所在盘。

.PARAMETER InstallDir
    直接指定安装目录，优先于 -Drive。

.PARAMETER NoPath
    只安装文件，不修改任何 PATH（自动测试用）。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\install.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\install.ps1 -Version v1.1.0 -Drive D

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\install.ps1 -ZipPath .\pytools-v1.1.0-windows-x64.zip
#>
[CmdletBinding()]
param(
    [string]$Version = 'latest',
    [string]$Repo = 'ycm50/pytools',
    [string]$DownloadUrl,
    [string]$ZipPath,
    [string]$Proxy,
    [string]$Drive,
    [string]$InstallDir,
    [switch]$NoPath
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

function Get-CurrentDrive {
    $p = (Get-Location).Path
    if ($p -match '^([A-Za-z]):') { return $Matches[1].ToUpper() + ':' }
    return $null
}

# 广播环境变量变更，让新开的资源管理器/终端感知
function Send-EnvironmentChanged {
    try {
        if (-not ([System.Management.Automation.PSTypeName]'Win32.NativeMethods').Type) {
            Add-Type -Namespace Win32 -Name NativeMethods -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("user32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
public static extern System.IntPtr SendMessageTimeout(System.IntPtr hWnd, uint Msg, System.UIntPtr wParam, string lParam, uint fuFlags, uint uTimeout, out System.UIntPtr lpdwResult);
'@
        }
        $result = [System.UIntPtr]::Zero
        [void][Win32.NativeMethods]::SendMessageTimeout([System.IntPtr]0xffff, 0x1A, [System.UIntPtr]::Zero,
            'Environment', 2, 5000, [ref]$result)
    } catch { }
}

function Test-IsAdmin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $pr = New-Object Security.Principal.WindowsPrincipal($id)
    return $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# 把目录插到系统 PATH 最顶端（需要管理员权限；未提权时请求 UAC 后重试）
function Update-SystemPathTop {
    param([string]$Dir)

    $target = $Dir.TrimEnd('\')
    $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    if (-not $machine) { $machine = '' }
    $parts = @($machine -split ';' | Where-Object { $_ -ne '' })

    $dupes = @($parts | Where-Object { $_.TrimEnd('\') -ieq $target })
    $alreadyTop = ($parts.Count -gt 0 -and ($parts[0].TrimEnd('\') -ieq $target))
    if ($alreadyTop -and $dupes.Count -eq 1) {
        Write-Host '系统 PATH 最顶端已是该目录，无需调整' -ForegroundColor Yellow
        return
    }

    $newPath = (@($target) + @($parts | Where-Object { $_.TrimEnd('\') -ine $target })) -join ';'

    if (Test-IsAdmin) {
        [Environment]::SetEnvironmentVariable('Path', $newPath, 'Machine')
        Send-EnvironmentChanged
        Write-Host "已将 $target 插入系统 PATH 最顶端（新开的终端生效）" -ForegroundColor Green
        return
    }

    # 未提权：生成临时脚本，通过 UAC 提权执行
    Write-Host '需要管理员权限才能修改系统 PATH，正在请求提权...' -ForegroundColor Yellow
    $elevScript = Join-Path (Get-TempBase) ('pytools_setpath_' + [guid]::NewGuid().ToString('N') + '.ps1')
    $template = @'
$ErrorActionPreference = 'Stop'
$machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
if (-not $machine) { $machine = '' }
$target = '__TARGET__'
$sep = [char]92
$parts = @($machine -split ';' | Where-Object { $_ -ne '' -and $_.TrimEnd($sep) -ine $target })
$newPath = (@($target) + $parts) -join ';'
[Environment]::SetEnvironmentVariable('Path', $newPath, 'Machine')
Write-Host '已写入系统 PATH 最顶端'
'@
    # 单引号转义，避免路径里含 ' 时把脚本拼坏
    $safeTarget = $target.Replace("'", "''")
    Set-Content -LiteralPath $elevScript -Value ($template.Replace('__TARGET__', $safeTarget)) -Encoding UTF8

    try {
        $proc = Start-Process -FilePath 'powershell.exe' `
            -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $elevScript) `
            -Verb RunAs -Wait -PassThru
        if ($proc.ExitCode -ne 0) {
            Write-Host '提权被取消或失败，未修改系统 PATH' -ForegroundColor Red
        } else {
            Send-EnvironmentChanged
            Write-Host "已将 $target 插入系统 PATH 最顶端（新开的终端生效）" -ForegroundColor Green
        }
    } catch {
        Write-Host ('提权失败，未修改系统 PATH: ' + $_.Exception.Message) -ForegroundColor Red
    } finally {
        Remove-Item -LiteralPath $elevScript -Force -ErrorAction SilentlyContinue
    }
}

# 清理旧版本可能留在用户 PATH 里的同名项
function Remove-UserPathEntry {
    param([string]$Dir)

    $target = $Dir.TrimEnd('\')
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    if (-not $userPath) { return }

    $parts = @($userPath -split ';' | Where-Object { $_ -ne '' })
    $kept = @($parts | Where-Object { $_.TrimEnd('\') -ine $target })
    if ($kept.Count -eq $parts.Count) { return }

    [Environment]::SetEnvironmentVariable('Path', ($kept -join ';'), 'User')
    Write-Host '已从用户 PATH 中移除同名旧项（已统一到系统 PATH）' -ForegroundColor Green
}

# 临时目录基路径：动态读取 TEMP / TMP，都不可用时回退系统 API
function Get-TempBase {
    foreach ($name in 'TEMP', 'TMP') {
        $value = [Environment]::GetEnvironmentVariable($name)
        if ($value) {
            $value = [Environment]::ExpandEnvironmentVariables($value)
            $value = $value.TrimEnd('\')
            if ($value -and (Test-Path -LiteralPath $value)) { return $value }
        }
    }
    return ([System.IO.Path]::GetTempPath()).TrimEnd('\')
}

function Get-EffectiveProxy {
    if ($Proxy) { return $Proxy }
    if ($env:HTTPS_PROXY) { return $env:HTTPS_PROXY }
    if ($env:HTTP_PROXY) { return $env:HTTP_PROXY }
    if ($env:https_proxy) { return $env:https_proxy }
    if ($env:http_proxy) { return $env:http_proxy }
    return $null
}

# ── 1. 解析安装目录 ─────────────────────────────────────────
if (-not $InstallDir -or $InstallDir -eq '') {
    $driveLetter = $null
    if ($Drive) {
        if ($Drive -match '^([A-Za-z]):?$') { $driveLetter = $Matches[1].ToUpper() } else { throw "无效的盘符: $Drive" }
    } else {
        $driveLetter = Get-CurrentDrive
        if (-not $driveLetter) { throw "无法识别当前工作目录所在盘，请用 -Drive 或 -InstallDir 指定" }
    }
    if (-not (Test-Path -LiteralPath ($driveLetter + '\'))) {
        throw "盘 $driveLetter 不存在"
    }
    $InstallDir = Join-Path ($driveLetter + '\') 'pytools'
}

Write-Host "安装目录: $InstallDir" -ForegroundColor Cyan

# ── 2. 取得发布包 ───────────────────────────────────────────
$effectiveProxy = Get-EffectiveProxy
$tempBase = Get-TempBase
if (-not $tempBase) { throw "无法获取临时目录（%TEMP%）" }
$tempRoot = Join-Path $tempBase ('pytools-install-' + [guid]::NewGuid().ToString('N'))

$package = $null
$tagName = ''

try {
    New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null

    if ($ZipPath) {
        if (-not (Test-Path -LiteralPath $ZipPath)) { throw "找不到本地发布包: $ZipPath" }
        $package = (Resolve-Path -LiteralPath $ZipPath).Path
        Write-Host "使用本地发布包: $package" -ForegroundColor Cyan
        $tagName = 'local'
    } else {
        $assetUrl = $DownloadUrl
        if (-not $assetUrl) {
            $apiUrl = if ($Version -eq 'latest') {
                "https://api.github.com/repos/$Repo/releases/latest"
            } else {
                "https://api.github.com/repos/$Repo/releases/tags/$Version"
            }
            Write-Host "正在查询 Release: $apiUrl" -ForegroundColor Cyan

            $params = @{
                Uri             = $apiUrl
                Headers         = @{ 'User-Agent' = 'pytools-installer' }
                UseBasicParsing = $true
                TimeoutSec      = 30
            }
            if ($effectiveProxy) { $params['Proxy'] = $effectiveProxy }

            try {
                $release = Invoke-RestMethod @params
            } catch {
                if ($Version -eq 'latest') {
                    throw "无法获取最新 Release（仓库 $Repo 可能还没有正式发布，或网络/代理不可用）: " + $_.Exception.Message
                }
                throw "找不到版本 $Version（仓库 $Repo），或网络/代理不可用: " + $_.Exception.Message
            }
            $tagName = $release.tag_name
            Write-Host "找到版本: $tagName" -ForegroundColor Green

            $assets = @($release.assets | Where-Object { $_.name -like '*.zip' })
            if ($assets.Count -eq 0) {
                throw "Release $tagName 中没有 zip 资产"
            }
            $preferred = @($assets | Where-Object { $_.name -like '*windows*' })
            if ($preferred.Count -gt 0) { $assets = $preferred }
            $assetUrl = $assets[0].browser_download_url
        }

        $package = Join-Path $tempRoot 'pytools-release.zip'
        Write-Host "正在下载: $assetUrl" -ForegroundColor Cyan
        $ok = $false
        for ($i = 1; $i -le 3; $i++) {
            try {
                $dlParams = @{
                    Uri             = $assetUrl
                    OutFile         = $package
                    Headers         = @{ 'User-Agent' = 'pytools-installer' }
                    UseBasicParsing = $true
                    TimeoutSec      = 300
                }
                if ($effectiveProxy) { $dlParams['Proxy'] = $effectiveProxy }
                Invoke-WebRequest @dlParams
                $ok = $true
                break
            } catch {
                Write-Host ("  第 " + $i + " 次下载失败: " + $_.Exception.Message) -ForegroundColor Yellow
                if ($i -lt 3) { Start-Sleep -Seconds 2 }
            }
        }
        if (-not $ok) { throw "下载发布包失败: $assetUrl" }
        Write-Host ("  下载完成: " + (Get-Item $package).Length + " 字节") -ForegroundColor Green
    }

    # ── 3. 识别格式并解压 ───────────────────────────────────
    $staging = Join-Path $tempRoot 'stage'
    New-Item -ItemType Directory -Path $staging -Force | Out-Null

    $magic = New-Object byte[] 4
    $fsIn = [System.IO.File]::OpenRead($package)
    $null = $fsIn.Read($magic, 0, 4)
    $fsIn.Close()

    if ($magic[0] -eq 0x50 -and $magic[1] -eq 0x4B) {
        Write-Host "发布包格式: ZIP" -ForegroundColor Cyan
        Expand-Archive -LiteralPath $package -DestinationPath $staging -Force
    } elseif ($magic[0] -eq 0x1F -and $magic[1] -eq 0x8B) {
        Write-Host "发布包格式: GZIP(tar.gz)" -ForegroundColor Cyan
        $tarPath = Join-Path $tempRoot 'pytools-release.tar'
        $inStream = [System.IO.File]::OpenRead($package)
        $gzStream = New-Object System.IO.Compression.GZipStream($inStream, [System.IO.Compression.CompressionMode]::Decompress)
        $outStream = [System.IO.File]::Create($tarPath)
        $gzStream.CopyTo($outStream)
        $outStream.Close()
        $gzStream.Close()
        $inStream.Close()
        & tar -xf $tarPath -C $staging
        if ($LASTEXITCODE -ne 0) { throw "tar 解压失败" }
    } else {
        throw "无法识别的发布包格式（既不是 ZIP 也不是 tar.gz）"
    }

    # 若包内只有一个目录且没有散落文件，则下钻
    $entries = @(Get-ChildItem -LiteralPath $staging -Force)
    $files = @($entries | Where-Object { -not $_.PSIsContainer })
    $dirs = @($entries | Where-Object { $_.PSIsContainer })
    if ($files.Count -eq 0 -and $dirs.Count -eq 1) {
        $staging = $dirs[0].FullName
    }

    $payload = @(Get-ChildItem -LiteralPath $staging -Force | Where-Object { -not $_.PSIsContainer })
    if ($payload.Count -eq 0) { throw "发布包内容为空" }

    # ── 4. 安装文件 ─────────────────────────────────────────
    if (-not (Test-Path -LiteralPath $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }

    $configBackup = $null
    $configPath = Join-Path $InstallDir 'config.json'
    if (Test-Path -LiteralPath $configPath) {
        $configBackup = Get-Content -LiteralPath $configPath -Raw
    }

    foreach ($f in $payload) {
        Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $InstallDir $f.Name) -Force
        Write-Host ("  已安装 " + $f.Name) -ForegroundColor Green
    }

    if ($configBackup) {
        Set-Content -LiteralPath $configPath -Value $configBackup -Encoding UTF8 -NoNewline
        Write-Host "  已保留原有 config.json（Python 版本记录不变）" -ForegroundColor Green
    }

    # ── 5. 校验关键二进制 ──────────────────────────────────
    $required = @('py.exe', 'pyins.exe')
    $missing = @($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $InstallDir $_)) })
    if ($missing.Count -gt 0) {
        throw ("安装不完整，缺少: " + ($missing -join ', '))
    }

    # ── 6. 初始 config.json ────────────────────────────────
    if (-not (Test-Path -LiteralPath $configPath)) {
        $root = (Split-Path -Path $InstallDir -Qualifier) + '\'
        $driveName = (Split-Path -Path $InstallDir -Qualifier)
        $config = [ordered]@{
            drive        = $driveName
            install_root = $root
            pythons      = @{}
        }
        $config | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $configPath -Encoding UTF8
        Write-Host "  已生成 config.json" -ForegroundColor Green
    }

    # ── 7. 把安装目录放到「系统 PATH」最顶端 ────────────────
    if (-not $NoPath) {
        Update-SystemPathTop -Dir $InstallDir
        Remove-UserPathEntry -Dir $InstallDir
    }

    Write-Host ""
    Write-Host "安装完成: $InstallDir (版本: $tagName)" -ForegroundColor Cyan
    Write-Host "下一步:"
    Write-Host "  cd 到目标盘任意目录后执行 pyins <version>，例如 pyins 3.12.1"
    Write-Host "  之后可用 py <version>、pip <version> ... 管理该 Python"
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
