@echo off
cd /d "%~dp0"
if not exist .venv\Scripts\python.exe (
  echo Run setup.bat first. It installs Python packages for Jarvis.
  pause
  exit /b 1
)
if not exist .env copy .env.example .env
echo Starting Jarvis Brain. Leave this window open.
echo On this laptop, open http://127.0.0.1:8787
.venv\Scripts\python.exe -m jarvis_brain
echo.
echo Jarvis Brain stopped.
pause
