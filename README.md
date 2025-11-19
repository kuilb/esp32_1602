# 1602A LCD WiFi

基于 ESP32-S3 的无线 1602A 显示屏，通过自定义协议接收数据帧并在 1602A LCD 上高速播放，内置 Web 配网和 OTA 升级。

---

## 最新Release信息

- Version: **1.0**
- 发布日期: **2025/11/20**
- 概要: 本次发布为 1.0 正式版本。

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

推荐使用 **PlatformIO**（VS Code 插件）打开本仓库。

1. 安装 VS Code 与 PlatformIO 插件。
2. 打开本项目根目录，PlatformIO 会自动识别 `platformio.ini`。
3. 连接 ESP32-S3 开发板到电脑。
4. 在 PlatformIO 中执行：
   - `Build`：编译固件
   - `Upload`：烧录固件
   - 执行 **FS Upload** 任务 ( 在命令行输入pio run --target uploadfs ) 

也可以使用命令行（在项目根目录）：

```powershell
pio run
pio run --target upload
pio run --target uploadfs
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
  - `playbuffer.*`：播放缓冲管理
  - `protocol.*`：自定义协议解析与封装
  - `weathertrans.*`：天气数据转换
  - `ota_manager.*`：OTA 升级逻辑
  - `web_component.*` / `web_setting.*`：Web 配网/管理相关组件
  - `kanamap.*`：假名字符映射表
- `src/utils/`：
  - `logger.*`：日志包装与调试输出
  - `memory_utils.*`：内存相关工具
- `website/`：Web 前端页面与静态资源

---

## 通信协议概览

当前固件通过 TCP 接收数据帧并写入播放缓冲，由 `protocol` 模块负责解析。详细 API 与帧格式仍在整理中，后续会在仓库中补充完整协议文档与示例。


---

## 历史更改记录

- 2025-11-20 (v1.0): 本次发布为 1.0 正式版本。

> Notes: 上述日志为本仓库近期主要改动的概览，更多细节请参阅 commit 历史和各个模块的注释。

---
