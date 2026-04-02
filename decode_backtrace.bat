@echo off
setlocal EnableExtensions

title ESP32 Backtrace Decoder

REM Usage:
REM   1) Run this .bat
REM   2) Paste ONE line: Backtrace: 0x42017b41:0x3fced520 ...
REM   3) Press Enter

set "ROOT=%~dp0"
REM Remove trailing backslash to avoid escaping the closing quote when passed as an argument
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set /p "BT=Paste ONE Backtrace line and press Enter: "
if "%BT%"=="" (
  echo.
  echo Empty input.
  exit /b 2
)

set "PS1=%TEMP%\decode_backtrace_%RANDOM%_%RANDOM%.ps1"
> "%PS1%" echo param([string]$root,[string]$line)
>> "%PS1%" echo $ErrorActionPreference = 'Stop'
>> "%PS1%" echo $root = [IO.Path]::GetFullPath($root)
>> "%PS1%" echo.
>> "%PS1%" echo function Find-Addr2Line {
>> "%PS1%" echo   $p1 = Join-Path $env:USERPROFILE '.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-addr2line.exe'
>> "%PS1%" echo   if (Test-Path $p1) { return $p1 }
>> "%PS1%" echo   $pkgRoot = Join-Path $env:USERPROFILE '.platformio\packages'
>> "%PS1%" echo   if (Test-Path $pkgRoot) {
>> "%PS1%" echo     $items = Get-ChildItem -Path $pkgRoot -Recurse -File -Filter 'xtensa-esp32s3-elf-addr2line.exe' -ErrorAction SilentlyContinue
>> "%PS1%" echo     if ($items -and $items.Count -gt 0) { return $items[0].FullName }
>> "%PS1%" echo   }
>> "%PS1%" echo   return $null
>> "%PS1%" echo }
>> "%PS1%" echo.
>> "%PS1%" echo function Find-FirmwareElf([string]$root) {
>> "%PS1%" echo   $pioBuild = Join-Path $root '.pio\build'
>> "%PS1%" echo   if (-not (Test-Path $pioBuild)) { return $null }
>> "%PS1%" echo   $elfs = Get-ChildItem -Path $pioBuild -Recurse -File -Filter 'firmware.elf' -ErrorAction SilentlyContinue
>> "%PS1%" echo   if (-not $elfs -or $elfs.Count -eq 0) { return $null }
>> "%PS1%" echo   $elfs = Sort-Object -InputObject $elfs -Property LastWriteTime -Descending
>> "%PS1%" echo   return $elfs[0].FullName
>> "%PS1%" echo }
>> "%PS1%" echo.
>> "%PS1%" echo $addr2line = Find-Addr2Line
>> "%PS1%" echo if (-not $addr2line) { throw 'Cannot find xtensa-esp32s3-elf-addr2line.exe. Please build once with PlatformIO first.' }
>> "%PS1%" echo $elf = Find-FirmwareElf $root
>> "%PS1%" echo if (-not $elf) { throw 'Cannot find firmware.elf under .pio\\build. Please run: pio run first.' }
>> "%PS1%" echo.
>> "%PS1%" echo $matches = [regex]::Matches($line,'0x[0-9a-fA-F]{8}')
>> "%PS1%" echo $set = New-Object 'System.Collections.Generic.HashSet[string]'
>> "%PS1%" echo foreach ($m in $matches) {
>> "%PS1%" echo   $a = $m.Value.ToLowerInvariant()
>> "%PS1%" echo   try {
>> "%PS1%" echo     $val = [Convert]::ToUInt32($a.Substring(2),16)
>> "%PS1%" echo     if ($val -ge 0x40000000) { [void]$set.Add($a) }
>> "%PS1%" echo   } catch {}
>> "%PS1%" echo }
>> "%PS1%" echo $addrs = New-Object string[] $set.Count
>> "%PS1%" echo $set.CopyTo($addrs)
>> "%PS1%" echo if (-not $addrs -or $addrs.Count -eq 0) { throw 'No valid code addresses found (^>= 0x40000000).' }
>> "%PS1%" echo.
>> "%PS1%" echo Write-Host ('ELF: ' + $elf) -ForegroundColor DarkGray
>> "%PS1%" echo Write-Host 'Decoded:' -ForegroundColor Green
>> "%PS1%" echo ^& $addr2line -pfiaC -e $elf @addrs

powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%" "%ROOT%" "%BT%"
set "PSERR=%ERRORLEVEL%"
del /q "%PS1%" >nul 2>nul
if not "%PSERR%"=="0" exit /b %PSERR%

if errorlevel 1 (
  echo.
  echo Failed to decode backtrace. ^(See error above^)
  exit /b 1
)

echo.
echo Done.
endlocal
