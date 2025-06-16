# 1602A LCD WiFi

基于 ESP32/ESP32-S3 的 无线1602A 显示屏，通过自定义协议接收数据并显示，内置配网模式，搭配 RGB LED 。

---

## 功能介绍

- 支持 1602A LCD 显示英文、假名和自定义字符
- 高性能(可以跑满1602A的83FPS刷新率)
- 支持WiFi 配网
- 按住中键+上键打开设置界面

---

## 硬件需求

- ESP32 或 ESP32-S3 开发板（支持 FFat 文件系统）
- 1602A LCD 显示屏（4位并行接口）
- RGB LED（WS2812 或类似）
- 多功能按键

---

## 软件依赖

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) / Arduino-ESP32 核心库
- [FastLED](https://github.com/FastLED/FastLED)
- FreeRTOS (集成于 ESP-IDF)
- FFat 文件系统支持

---

## 使用说明

1. **编译与上传**  
   使用 PlatformIO 或 Arduino IDE 编译并上传代码到 ESP32 开发板。

2. **首次启动**  
   如果没有保存的 WiFi 信息，设备会自动进入 SoftAP 配网模式。  
   连接热点 `1602A_Config`，访问显示的 IP 配置 WiFi。

3. **配网模式**  
   进入配网模式后，LCD 显示提示信息，RGB LED 亮紫灯。

4. **连接 WiFi**  
   读取保存的 WiFi 信息自动连接，成功连接后显示 SSID 和 IP，LED 亮绿灯。

5. **数据接收与显示**  
   通过 TCP 接收数据包，根据协议解析并显示到 LCD，LED 显示连接状态颜色。

---

## 代码结构

- `lcd_*` ：LCD 显示控制相关函数
- `wifi_*` ：WiFi连接与配网逻辑
- `network.*` ：TCP服务器及数据接收处理
- `rgb_led.*` ：RGB LED控制任务及线程安全接口
- `button.*` ：按键检测任务
- `main.cpp` ：主程序入口及初始化

---

## 协议说明

-未来将会更新API

---

## 贡献

欢迎提出 Issue 或 Pull Request，一起完善项目。

项目维护者：Kulib

