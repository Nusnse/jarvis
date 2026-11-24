Set-Location -Path $PSScriptRoot
if (-not (Test-Path .\.venv\Scripts\python.exe)) {
  Write-Host "Run setup.ps1 first."
  exit 1
}
if (-not (Test-Path .env)) { Copy-Item .env.example .env }
Write-Host "Starting Jarvis Brain. Leave this window open."
Write-Host "On this laptop, open http://127.0.0.1:8787"
& .\.venv\Scripts\python.exe -m jarvis_brain
