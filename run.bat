@echo off
choice /c rda /n /m "build release/debug/all?(r/d/a):"
if errorlevel 3 (
    python .\script\set_version.py release
    pio run -e esp32s3-1602
    python .\script\set_version.py debug
    pio run -e esp32s3-1602-debug
) else if errorlevel 2 (
    python .\script\set_version.py debug
    pio run -e esp32s3-1602-debug
) else if errorlevel 1 (
    python .\script\set_version.py release
    pio run -e esp32s3-1602
)

echo upload?(y/n):
choice /c yn /n /m "y/n:"

if errorlevel 2 (
    goto end
) else (
    choice /c rd /n /m "upload release/debug?(r/d):"
        if errorlevel 2 (
            pio run -e esp32s3-1602-debug --target upload
        ) else (
            pio run -e esp32s3-1602 --target upload
        )
)
:end