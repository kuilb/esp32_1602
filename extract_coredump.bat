@echo off
setlocal EnableExtensions EnableDelayedExpansion

title ESP32S3 CoreDump Extractor

REM Usage:
REM   extract_coredump.bat COMx [baud]
REM Example:
REM   extract_coredump.bat COM3 921600
REM
REM Output:
REM   coredump_yyyyMMdd_HHmmss.bin (in project root)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "PART=%ROOT%\partitions.csv"
if not exist "%PART%" (
  echo [ERR] Cannot find partitions.csv at: "%PART%"
  exit /b 2
)

REM Get offset and size from partitions.csv line that starts with "coredump,"
set "OFFSET="
set "SIZE="
for /f "usebackq tokens=1-5 delims=," %%A in (`findstr /i /b "coredump," "%PART%"`) do (
  set "OFFSET=%%D"
  set "SIZE=%%E"
)

if "%OFFSET%"=="" (
  echo [ERR] Cannot find coredump partition in partitions.csv
  echo       Expected a line like: coredump, data, coredump,0x510000,  0x10000
  exit /b 3
)

REM Trim spaces
set "OFFSET=%OFFSET: =%"
set "SIZE=%SIZE: =%"

REM Port and baud
set "PORT=%~1"
if "%PORT%"=="" (
  set /p "PORT=Enter serial port (e.g. COM3): "
)
if "%PORT%"=="" (
  echo [ERR] Serial port is required.
  exit /b 4
)

set "BAUD=%~2"
if "%BAUD%"=="" set "BAUD=921600"

REM Timestamped output filename via PowerShell to avoid locale issues
for /f "usebackq delims=" %%T in (`powershell -NoProfile -Command "(Get-Date).ToString('yyyyMMdd_HHmmss')"`) do set "TS=%%T"
set "OUT=%ROOT%\coredump_!TS!.bin"

REM Prefer PlatformIO-managed Python + esptool if available
set "PY=%USERPROFILE%\.platformio\penv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

set "ESPTOOL=%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py"
if not exist "%ESPTOOL%" set "ESPTOOL=%ROOT%\.pio\packages\tool-esptoolpy\esptool.py"

echo.
echo [INFO] partitions.csv : "%PART%"
echo [INFO] coredump offset: %OFFSET%
echo [INFO] coredump size  : %SIZE%
echo [INFO] port/baud      : %PORT% / %BAUD%
echo [INFO] output         : "%OUT%"
echo.

if exist "%ESPTOOL%" (
  "%PY%" "%ESPTOOL%" --chip esp32s3 --port "%PORT%" --baud %BAUD% read_flash %OFFSET% %SIZE% "%OUT%"
) else (
  echo [WARN] Cannot find PlatformIO esptool.py at:
  echo        - "%USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py"
  echo        - "%ROOT%\.pio\packages\tool-esptoolpy\esptool.py"
  echo [INFO] Trying: "%PY%" -m esptool
  "%PY%" -m esptool --chip esp32s3 --port "%PORT%" --baud %BAUD% read_flash %OFFSET% %SIZE% "%OUT%"
)

if errorlevel 1 (
  echo.
  echo [ERR] Failed to read coredump from device.
  echo       Tips:
  echo       - Make sure the COM port is correct and not occupied by serial monitor.
  echo       - Unplug/replug USB and retry.
  echo       - If auto-reset fails, hold BOOT while starting the command.
  exit /b 5
)

REM Quick sanity check: if the partition is still all 0xFF, no coredump was stored.
REM (Optional) You can quickly verify whether the dump is empty:
REM   powershell -NoProfile -Command "Format-Hex -Path '<file>' | Select-Object -First 4"

echo.
echo [OK] Core dump extracted to: "%OUT%"
endlocal
