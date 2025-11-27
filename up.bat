@echo off
echo ========================================
echo ESP32-S3 快速编译烧录工具
echo ========================================
echo.

REM 检查是否存在文件系统烧录标记文件
set "FS_FLAG_FILE=.pio\.fs_uploaded"

if not exist "%FS_FLAG_FILE%" (
    echo [INFO] 检测到首次烧录或文件系统未上传
    echo [INFO] 准备烧录文件系统...
    echo.
    
    pio run --target uploadfs
    
    if errorlevel 1 (
        echo.
        echo [ERROR] 文件系统烧录失败！
        pause
        exit /b 1
    ) else (
        echo.
        echo [SUCCESS] 文件系统烧录成功
        REM 创建标记文件
        if not exist ".pio" mkdir .pio
        echo %date% %time% > "%FS_FLAG_FILE%"
        echo.
    )
) else (
    echo [INFO] 检测到文件系统已烧录（标记文件存在）
    echo [INFO] 跳过文件系统烧录步骤
    echo.
    
    choice /c yn /n /m "是否重新烧录文件系统? (y/n): "
    if errorlevel 2 (
        echo [INFO] 跳过文件系统烧录
        echo.
    ) else (
        echo [INFO] 重新烧录文件系统...
        echo.
        pio run --target uploadfs
        
        if errorlevel 1 (
            echo.
            echo [ERROR] 文件系统烧录失败！
            pause
            exit /b 1
        ) else (
            echo.
            echo [SUCCESS] 文件系统烧录成功
            echo %date% %time% > "%FS_FLAG_FILE%"
            echo.
        )
    )
)

echo ========================================
echo 开始编译并烧录固件...
echo ========================================
echo.

REM 生成版本信息
python .\script\set_version.py release

echo.
echo [INFO] 按任意键开始烧录固件...
pause

REM 编译并烧录
pio run -e esp32s3-1602 --target upload

if errorlevel 1 (
    echo.
    echo [ERROR] 固件烧录失败！
    pause
    exit /b 1
) else (
    echo.
    echo ========================================
    echo [SUCCESS] 烧录完成！
    echo ========================================
    pause
)