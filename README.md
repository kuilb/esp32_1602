# 1602A LCD WiFi

基于 ESP32-S3 的无线 1602A 显示屏，通过自定义协议接收数据帧并在 1602A LCD 上高速播放，内置 Web 配网和 OTA 升级。

---

## 最新Release信息

- Version: **1.0.3(10A021)**
- 发布日期: **2025/11/27**
- 概要: 实现配置管理器框架并添加完整测试套件
### 更新内容
- 新功能: 新增配置管理器基类(ConfigManager)，提供统一的配置文件读写接口
- 新功能: 实现WiFi配置管理器(WifiConfigManager)，支持SSID和密码的持久化存储
- 新功能: 实现和风天气认证配置管理器(QWeatherAuthConfigManager)，管理API认证信息和位置配置
- 重构: 重构web_setting.cpp，使用QWeatherAuthConfigManager替代原有的配置文件操作
- 重构: 添加统一的图标定义和工具函数(icons.h/cpp)
- 重构: 配置文件改用JSON存储
- 重构: 配置文件操作添加详细的错误处理和错误信息返回

- 新功能: 添加测试初始化框架(test_init.h)，提供统一的测试环境管理
- 新功能: ConfigManager基类测试(10个测试用例)
- 新功能: WiFiConfigManager测试(20个测试用例)
- 新功能: QWeatherAuthConfigManager测试(30个测试用例)


---

## 功能特性

- 支持 1602A LCD 显示英文、假名和自定义字符（内置假名映射）
- 高性能刷新（可跑满 1602A 的约 83 FPS 刷新极限）
- 内置多页面菜单系统（时钟、天气、设置等）
- 支持 WiFi 配网和 Web 配置页
- 支持 OTA 固件升级
- RGB LED 实时显示状态
- 多功能按键：上下左右 + 中键，支持无线串流按键

---

## 硬件需求

- ESP32 或 ESP32-S3 开发板（Release采用 SPIFFS）
- 1602A LCD 显示屏（4-bit 并行接口）
- RGB LED（单颗WS2812 或兼容灯珠）
- 5 个按键：上 / 下 / 左 / 右 / 中

---

## 软件依赖

- Arduino-ESP32 核心库（本工程使用 PlatformIO + Arduino 框架）
- [FastLED](https://github.com/FastLED/FastLED)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [libsodium](https://github.com/jedisct1/libsodium)
- [zlib_turbo](https://github.com/bitbank2/zlib_turbo)

---

## 编译与烧录

本项目使用 **PlatformIO** 开发框架，支持 ESP32-S3-N8R8（8MB Flash + 8MB PSRAM）开发板。

### 环境准备

1. 安装 [VS Code](https://code.visualstudio.com/)
2. 安装 [PlatformIO IDE 插件](https://platformio.org/install/ide?install=vscode)
3. 安装 Python 3.x（用于版本管理脚本）

### 硬件要求

- **开发板**：ESP32-S3-DevKitC-1 或兼容板
- **Flash**：8MB（N8）
- **PSRAM**：8MB OPI PSRAM（R8）
- **USB 连接**：用于烧录和串口监视

### 编译环境

项目配置了两个编译环境：

#### 1. Release 环境 (`esp32s3-1602`)

- **优化级别**：默认优化
- **日志级别**：INFO
- **用途**：正式发布版本
- **编译命令**：
  ```powershell
  python .\script\set_version.py release
  pio run -e esp32s3-1602
  ```

#### 2. Debug 环境 (`esp32s3-1602-debug`)

- **优化级别**：-O0（无优化）
- **日志级别**：VERBOSE
- **调试信息**：完整 GDB 调试符号
- **用途**：开发调试
- **编译命令**：
  ```powershell
  python .\script\set_version.py debug
  pio run -e esp32s3-1602-debug
  ```

### 快速开始

#### 方法1：使用自动化脚本（推荐）

在项目根目录执行：

```powershell
# Windows
.\run.bat

# 或简化版（快速编译上传）
.\up.bat
```

**run.bat** 交互式菜单：
- 选择编译版本（Release/Debug/All）
- 自动生成版本信息
- 选择是否立即烧录

**up.bat** 快速流程：
- 自动生成版本信息
- 编译 Release 版本
- 立即烧录到开发板

#### 方法2：手动编译

```powershell
# 1. 生成版本信息
python .\script\set_version.py release

# 2. 编译固件
pio run -e esp32s3-1602

# 3. 烧录固件
pio run -e esp32s3-1602 --target upload

# 4. 烧录文件系统（首次使用必须执行）
pio run --target uploadfs
```

### 版本管理

项目使用自动版本号生成系统：

- **版本号格式**：`主版本.次版本.修订版(构建版本)`
- **示例**：`1.0.3(10D5021)`
- **版本文件**：`version.txt`（自动生成）
- **版本注入**：编译时通过 `-D` 参数注入到固件

**版本脚本**：
```powershell
# 生成 Release 版本号
python .\script\set_version.py release

# 生成 Debug 版本号
python .\script\set_version.py debug

# 仅生成版本号（不修改环境）
python .\script\genrate_version.py
```

### 文件系统说明

项目使用 **SPIFFS** 文件系统存储配置和 Web 资源：

- **分区大小**：见 `partitions.csv`
- **数据目录**：`data/`
- **包含文件**：Web 页面、配置文件等

**首次烧录或更新 Web 资源时必须执行**：
```powershell
pio run --target uploadfs
```

### 常见问题

**Q: 烧录失败 "Failed to connect"**  
A: 
- 检查 USB 连接
- 尝试按住 BOOT 按钮同时按 RESET 按钮进入下载模式
- 检查 COM 端口是否被占用

**Q: 编译错误 "No such file or directory"**  
A: 
- 确认已安装所有依赖库
- 执行 `pio pkg install` 重新安装依赖

**Q: PSRAM 初始化失败**  
A: 
- 确认硬件为 ESP32-S3-N8R8
- 检查 `platformio.ini` 中 PSRAM 配置是否正确

**Q: 单元测试如何运行？**  
A: 
```powershell
# 运行所有测试
pio test

# 运行特定测试
pio test -f test_config_manager
pio test -f test_wifi_config
pio test -f test_qweather_config
```

### 输出文件

编译成功后，固件位于：
```
.pio/build/esp32s3-1602/firmware.bin
.pio/build/esp32s3-1602-debug/firmware.bin
```

---

## 代码结构

- `src/main.cpp`：主程序入口、系统初始化与主循环
- `src/hardware/`：
  - `lcd_driver.*`：1602A LCD 驱动
  - `rgb_led.*`：RGB LED 控制与亮度/颜色更新
  - `button.*`：按键扫描与事件派发
- `src/applications/`：
  - `menu.*`：主菜单与界面切换
  - `clock.*`：时钟应用
  - `weather.*`：天气显示应用
  - `badappleplayer.*`：Bad Apple 播放器
  - `about.*`：关于信息界面
- `src/connectivity/`：
  - `wifi_config.*`：WiFi 配网与状态机
  - `network.*`：TCP 网络收发与客户端管理
  - `jwt_auth.*`：JWT 认证相关
- `src/services/`：
  - `config_manager.*`：配置管理器基类，提供统一的配置文件读写接口
  - `wifi_config_manager.*`：WiFi 配置管理器，支持 SSID 和密码的持久化存储
  - `qweather_auth_config_manager.*`：和风天气认证配置管理器，管理 API 认证信息和位置配置
  - `playbuffer.*`：播放缓冲管理
  - `protocol.*`：自定义协议解析与封装
  - `ota_manager.*`：OTA 升级逻辑
  - `web_setting.*`：Web 设置相关组件
  - `kanamap.*`：假名字符映射表
- `src/ui/`：
  - `icons.*`：图标定义与工具函数
- `src/utils/`：
  - `logger.*`：日志包装与调试输出
  - `memory_utils.*`：内存相关工具
- `test/`：单元测试代码
  - `common/test_init.h`：测试初始化框架
  - `test_config_manager/`：ConfigManager 基类测试
  - `test_wifi_config/`：WiFiConfigManager 测试
  - `test_qweather_config/`：QWeatherAuthConfigManager 测试
- `website/`：Web 前端页面与静态资源

---

## 通信协议概览

本固件通过 TCP 接收数据帧并在 1602A LCD 上高速播放，由 `protocol` 模块负责解析。协议支持普通字符、UTF-8 假名和自定义点阵字符的显示。

### 协议格式

#### 数据包结构

| 字节位置 | 字段名称 | 长度 | 说明 |
|---------|---------|------|------|
| 0-1 | 协议头 | 2 字节 | 固定值：`0xAA 0x55` |
| 2 | 数据长度 | 1 字节 | 数据体长度（不含头部） |
| 3-4 | 帧间隔 | 2 字节 | 帧率间隔（毫秒），高字节在前 |
| 5-N | 数据体 | 可变 | 显示内容数据 |

#### 数据体格式

数据体由多个数据块组成，每个数据块以标志字节开头：

**1. 普通字符块（标志：0x00）**

```
[0x00] [字符数据]
```

- 支持 ASCII 字符直接显示
- 支持 UTF-8 编码的假名字符（3字节）
  - 格式：`0xE3 [byte1] [byte2]`
  - 通过内置 `kanaMap` 映射表转换为 1602A 可显示字符
  - 未定义的UTF8字符自动显示为空格

**2. 自定义字符块（标志：0x01）**

```
[0x01] [8字节点阵数据]
```

- 8 字节点阵数据定义 8×5 像素的自定义字符
- 自动循环使用 8 个自定义字符槽（CGRAM 0-7）
- 每字节对应一行，低 5 位有效

### 使用示例

#### 示例1：显示普通文本 "Hello"

```
AA 55 06 00 64 00 48 00 65 00 6C 00 6C 00 6F
```

- `AA 55`：协议头
- `06`：数据长度（6字节）
- `00 64`：帧间隔 100ms
- `00 48`：字符 'H'
- `00 65`：字符 'e'
- `00 6C`：字符 'l'
- `00 6C`：字符 'l'
- `00 6F`：字符 'o'

#### 示例2：显示假名 "あ"

```
AA 55 04 00 64 00 E3 81 82
```

- `00 E3 81 82`：UTF-8 编码的 "あ"

#### 示例3：显示自定义字符（笑脸）

```
AA 55 09 00 64 01 00 0A 0A 00 11 0E 00 00
```

- `01`：自定义字符标志
- `00 0A 0A 00 11 0E 00 00`：8×5 点阵数据

### 性能特性

- 最大支持帧率：约 **83 FPS**（1602A 硬件极限）
- 实时帧率统计：每秒计算并输出当前 FPS
- 批量字符输出优化：普通字符缓冲后批量发送
- 循环自定义字符槽：支持连续显示多个自定义字符

### 协议特性

-  支持 ASCII 字符
-  支持 UTF-8 假名字符（日文平假名/片假名）
-  支持自定义 8×5 点阵字符
-  自动填充空格至满屏（2 行 × 16 列 = 32 字符）
-  安全边界检查，防止数组越界
-  循环计数器保护，防止死循环
-  实时帧率监控

---

## 历史更改记录

- 2025-11-20 (v1.0.0): 本次发布为 1.0.0 正式版本。
- 2025-11-22 (v1.0.1): 将web完全前后端分离并增强健壮性
- 2025-11-25 (v1.0.2): 重构WiFi配置处理并改进版本管理
- 2025-11-27 (v1.0.3): 实现配置管理器框架并添加完整测试套件

> Notes: 上述日志为本仓库近期主要改动的概览，更多细节请参阅 commit MSG和各个模块的注释。

---
