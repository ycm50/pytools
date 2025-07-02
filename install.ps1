param (
    [string]$reqFile = "requirements.txt"
)

if (Test-Path $reqFile) {
    Write-Output "ڰװ..."
    pip install -r $reqFile
    if ($LASTEXITCODE -eq 0) {
        Write-Output "װɹ"
    } else {
        Write-Output "װʧܣ룺$LASTEXITCODE"
    }
} else {
    Write-Output "ļ $reqFile ڣ"
}