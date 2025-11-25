@echo off
python .\script\set_version.py
pause
pio run --target upload