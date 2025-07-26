param(
    [string]$port
)

if ($port) {
    $env:http_proxy = "127.0.0.1:$port"
    $env:https_proxy = "127.0.0.1:$port"
} else {
    $env:http_proxy = ""
    $env:https_proxy = ""
}
# �����ǰ��������ֵ��ȷ��
Write-Output "http_proxy: $($env:http_proxy)"
Write-Output "https_proxy: $($env:https_proxy)"