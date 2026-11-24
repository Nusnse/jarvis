# Installs Jarvis Brain into a private folder so it does not touch other Python projects.
Set-Location -Path $PSScriptRoot
Write-Host "Installing Jarvis Brain..."
$python = $null
if (Get-Command py -ErrorAction SilentlyContinue) { $python = "py -3" }
elseif (Get-Command python -ErrorAction SilentlyContinue) { $python = "python" }
else {
  Write-Host "Python was not found. Install it from https://www.python.org/downloads/"
  Write-Host "On the first installer screen, tick Add python.exe to PATH."
  exit 1
}
Invoke-Expression "$python -m venv .venv"
& .\.venv\Scripts\python.exe -m pip install --upgrade pip
& .\.venv\Scripts\python.exe -m pip install -r requirements.txt
if (-not (Test-Path .env)) { Copy-Item .env.example .env }
Write-Host ""
Write-Host "Jarvis Brain is installed. Next, run:  .\run.ps1"
