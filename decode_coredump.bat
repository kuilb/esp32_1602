@echo off
setlocal EnableExtensions EnableDelayedExpansion

title ESP32S3 CoreDump Decoder

REM Usage:
REM   decode_coredump.bat [coredump_bin_path] [firmware_elf_path]
REM Examples:
REM   decode_coredump.bat
REM   decode_coredump.bat coredump_20251218_140352.bin
REM   decode_coredump.bat coredump_20251218_140352.bin .pio\build\esp32s3-1602\firmware.elf

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

REM Prefer PlatformIO-managed Python
set "PY=%USERPROFILE%\.platformio\penv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

REM Locate espcoredump.py (from PlatformIO's framework-espidf if present)
set "ESPCOREDUMP=%USERPROFILE%\.platformio\packages\framework-espidf\components\espcoredump\espcoredump.py"
if not exist "%ESPCOREDUMP%" (
  if not "%IDF_PATH%"=="" (
    set "ESPCOREDUMP=%IDF_PATH%\components\espcoredump\espcoredump.py"
  )
)
if not exist "%ESPCOREDUMP%" (
  echo [ERR] Cannot find espcoredump.py
  echo       Searched:
  echo       - "%USERPROFILE%\.platformio\packages\framework-espidf\components\espcoredump\espcoredump.py"
  echo       - "%%IDF_PATH%%\components\espcoredump\espcoredump.py"
  exit /b 2
)

REM Ensure esp-coredump python package is installed
"%PY%" -c "import esp_coredump" >nul 2>nul
if errorlevel 1 (
  echo [INFO] Installing python package: esp-coredump
  "%PY%" -m pip install -U esp-coredump
  if errorlevel 1 (
    echo [ERR] Failed to install esp-coredump.
    exit /b 3
  )
)

REM Resolve core dump bin
set "CORE=%~1"
if "%CORE%"=="" (
  for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "Get-ChildItem -Path '%ROOT%' -Filter 'coredump_*.bin' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName"`) do set "CORE=%%F"
)
if "%CORE%"=="" (
  echo [ERR] Core dump file not specified and no coredump_*.bin found in project root.
  exit /b 4
)
if not exist "%CORE%" (
  REM maybe user passed relative path
  if exist "%ROOT%\%CORE%" set "CORE=%ROOT%\%CORE%"
)
if not exist "%CORE%" (
  echo [ERR] Core dump file not found: "%CORE%"
  exit /b 5
)

REM Resolve firmware ELF candidates (auto retry on SHA mismatch)
set "ELF_ARG=%~2"
set "CANDIDATES_TMP=%TEMP%\esp32_coredump_elves_%RANDOM%%RANDOM%.lst"
type nul >"%CANDIDATES_TMP%"

if not "%ELF_ARG%"=="" (
  set "ELF=%ELF_ARG%"
  if not exist "%ELF%" (
    if exist "%ROOT%\%ELF_ARG%" set "ELF=%ROOT%\%ELF_ARG%"
  )
  if not exist "%ELF%" (
    echo [ERR] firmware.elf not found: "%ELF_ARG%"
    del "%CANDIDATES_TMP%" >nul 2>nul
    exit /b 7
  )
  >>"%CANDIDATES_TMP%" echo %ELF%
)

REM Append discovered firmware.elf files (latest first)
powershell -NoProfile -Command "Get-ChildItem -Path '%ROOT%\.pio\build' -Recurse -Filter firmware.elf -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -ExpandProperty FullName" >>"%CANDIDATES_TMP%"

REM Locate gdb (optional but recommended)
set "GDB=%USERPROFILE%\.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-gdb.exe"
if not exist "%GDB%" set "GDB="

echo.
echo [INFO] core dump : "%CORE%"
echo [INFO] tool      : "%ESPCOREDUMP%"
if not "%GDB%"=="" echo [INFO] gdb       : "%GDB%"
echo.

set "FOUND_ELF="
set "DECODE_OK="
set "MISMATCH_SEEN="

for /f "usebackq delims=" %%E in ("%CANDIDATES_TMP%") do (
  if "%%E"=="" (
    rem skip empty
  ) else if not exist "%%E" (
    rem skip missing
  ) else (
    set "ELF=%%E"
    if not defined FOUND_ELF set "FOUND_ELF=1"
    echo [INFO] trying ELF : "%%E"
    call :DecodeWithElf "%CORE%" "%%E"
    if "!RC!"=="0" (
      set "DECODE_OK=1"
      goto :post_decode
    )
    if "!MISMATCH!"=="1" (
      set "MISMATCH_SEEN=1"
      echo [WARN] SHA mismatch for "%%E". Trying next candidate...
      if exist "!LOG!" del "!LOG!" >nul 2>nul
      set "LOG="
      set "MISMATCH="
      set "RC="
    ) else (
      echo.
      echo [ERR] Failed to decode core dump.
      if exist "!LOG!" del "!LOG!" >nul 2>nul
      goto :post_decode
    )
  )
)

:post_decode
if exist "%CANDIDATES_TMP%" del "%CANDIDATES_TMP%" >nul 2>nul

if not defined FOUND_ELF (
  echo [ERR] Cannot find firmware.elf under .pio\build. Please build once via PlatformIO.
  exit /b 6
)

if defined DECODE_OK (
  endlocal
  exit /b 0
)

if defined MISMATCH_SEEN (
  echo [ERR] Tried all firmware.elf files but app SHA did not match the coredump. Rebuild the firmware that produced the crash or supply its firmware.elf explicitly.
) else (
  echo [ERR] Failed to decode core dump. Make sure the ELF matches the firmware that produced the crash.
)
endlocal
exit /b 8

:DecodeWithElf
set "MISMATCH="
set "LOG=%TEMP%\esp32_coredump_decode_%RANDOM%%RANDOM%.log"

REM Note: extracted coredump_*.bin is raw flash data, so use --core-format raw
if "%GDB%"=="" (
  "%PY%" "%ESPCOREDUMP%" --chip esp32s3 info_corefile --core "%~1" --core-format raw "%~2" >"%LOG%" 2>&1
) else (
  "%PY%" "%ESPCOREDUMP%" --chip esp32s3 info_corefile --core "%~1" --core-format raw --gdb "%GDB%" "%~2" >"%LOG%" 2>&1
)
set "RC=%ERRORLEVEL%"
type "%LOG%"

if not "%RC%"=="0" (
  findstr /c:"CURRENT THREAD STACK" "%LOG%" >nul 2>nul
  if not errorlevel 1 (
    echo.
    echo [WARN] espcoredump returned error, but a stack trace was produced above; likely due to Windows GDB instability. Treating as success.
    del "%LOG%" >nul 2>nul
    echo.
    echo [OK] Done.
    set "RC=0"
    exit /b
  )

  findstr /c:"Invalid application image for coredump" "%LOG%" >nul 2>nul
  if not errorlevel 1 set "MISMATCH=1"
  findstr /c:"SHA256(" "%LOG%" >nul 2>nul
  if not errorlevel 1 set "MISMATCH=1"
  exit /b
)

del "%LOG%" >nul 2>nul
echo.
echo [OK] Done.
exit /b
