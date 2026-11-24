@echo off
cd /d "%~dp0"
echo Installing Jarvis Brain...
where py >nul 2>&1
if %errorlevel%==0 (
  py -3 -m venv .venv
) else (
  python -m venv .venv
)
if not exist .venv\Scripts\python.exe (
  echo.
  echo Python was not found. Install Python 3 from https://www.python.org/downloads/
  echo On the first installer screen, tick "Add python.exe to PATH".
  pause
  exit /b 1
)
call .venv\Scripts\python.exe -m pip install --upgrade pip
call .venv\Scripts\python.exe -m pip install -r requirements.txt
if not exist .env copy .env.example .env
echo.
echo Jarvis Brain is installed.
echo Next: double-click run.bat and leave that window open.
pause
