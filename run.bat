@echo off
python .\script\set_version.py
pause
pio run
echo upload?(y/n):
choice /c yn /n /m "y/n:"

if errorlevel 2 (
    goto end
) else (
    pio run --target upload
)
:end