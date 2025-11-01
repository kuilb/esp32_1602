# ESP32 统一日志系统

## 概述

本项目实现了一个专业的统一日志系统，用于替换分散在各个模块中的 `Serial.print` 调用，提供更好的调试和生产环境控制。

## 功能特性

### 1. 多级日志支持
- **ERROR**: 错误信息（红色重要）
- **WARN**: 警告信息（黄色注意）
- **INFO**: 一般信息（白色正常）
- **DEBUG**: 调试信息（灰色详细）
- **VERBOSE**: 详细日志（最详细）

### 2. 模块化管理
系统支持按模块分别控制日志级别：
- SYSTEM: 系统初始化和核心功能
- WIFI: WiFi连接和配置
- TIME_SYNC: 时间同步功能
- WEATHER: 天气数据获取
- MENU: 菜单和用户界面
- LCD: LCD显示驱动
- BUTTON: 按键处理
- WEATHER: 天气API、城市搜索和数据获取
- MENU: 菜单系统
- LCD: LCD显示控制
- BUTTON: 按键处理
- JWT: JWT认证和令牌管理
- WEB: Web服务器和配置界面
- NETWORK: 网络通信
- MEMORY: 内存管理和RAII
- RGB: RGB LED控制

### 3. 智能格式化
- 自动时间戳（运行时间）
- 模块标识前缀
- 级别标识符
- 自动换行处理

## 使用方法

### 基本初始化
```cpp
// 在main.cpp的setup()函数中初始化
Logger::init(LOG_LEVEL_INFO);  // 设置全局日志级别
```

### 基本日志调用
```cpp
// 使用通用日志宏
LOG_ERROR(LOG_MODULE_WIFI, "连接失败: %s", ssid.c_str());
LOG_WARN(LOG_MODULE_SYSTEM, "内存不足，剩余: %d bytes", freeHeap);
LOG_INFO(LOG_MODULE_WEATHER, "天气数据已更新: %s", weather.c_str());
LOG_DEBUG(LOG_MODULE_TIME_SYNC, "NTP响应时间: %lu ms", responseTime);

// 使用便捷模块宏
LOG_WIFI_ERROR("WiFi连接超时");
LOG_WEATHER_INFO("获取天气数据成功");
LOG_WEATHER_INFO("City search request URL: %s", url.c_str());
LOG_WEATHER_DEBUG("City search JWT token generated, length: %d", jwtToken.length());
LOG_WEATHER_ERROR("Failed to save JWT config - cannot open file");
LOG_TIME_DEBUG("时间同步进度: %d%%", progress);
LOG_SYSTEM_WARN("系统温度过高: %d°C", temp);
LOG_JWT_INFO("JWT token generated successfully, length: %d", jwt.length());
LOG_MEMORY_DEBUG("Allocated %u bytes for %s", size, name);

// 内存监控专用宏
LOG_MEMORY_INFO();  // 显示当前内存使用情况
```

### 高级配置
```cpp
// 设置全局日志级别
Logger::setGlobalLevel(LOG_LEVEL_DEBUG);

// 设置特定模块的日志级别
Logger::setModuleLevel(LOG_MODULE_WIFI, LOG_LEVEL_ERROR);     // 只显示WiFi错误
Logger::setModuleLevel(LOG_MODULE_WEATHER, LOG_LEVEL_VERBOSE); // 天气模块显示所有日志

// 内存监控
LOG_MEMORY_INFO();  // 显示当前内存使用情况
```

## 输出格式示例

```
=== Logger System Initialized ===
[INIT] Global log level: INFO
==================================
[00:00:03.245] [INFO][SYSTEM] Wireless 1602A by Kulib
[00:00:03.250] [INFO][SYSTEM] 2025/11/01
[00:00:03.255] [INFO][SYSTEM] 开始初始化...
[00:00:03.890] [INFO][SYSTEM] SPIFFS已挂载
[00:00:03.895] [DEBUG][SYSTEM]   FILE: /wifi.txt	SIZE: 45
[00:00:04.120] [INFO][JWT] JWT 配置已加载
[00:00:04.125] [INFO][WIFI] 正在连接: MyWiFi
[00:00:06.340] [INFO][TIME_SYNC] Initializing non-blocking time sync...
[00:00:06.345] [DEBUG][TIME_SYNC] NTP servers configured, waiting for response...
[00:00:08.123] [INFO][TIME_SYNC] NTP Time Synced successfully!
[00:00:08.128] [INFO][TIME_SYNC] Current time: 2025-01-15 14:23:45
[00:00:10.355] [INFO][JWT] JWT token generated successfully, length: 284
[00:00:10.567] [INFO][WEATHER] API config OK, fetching weather...
[00:00:10.572] [INFO][MEMORY] Weather fetch start - Free: 245760 bytes, Max Alloc: 110592 bytes, Min Free: 234568 bytes
[00:00:10.577] [DEBUG][MEMORY] Allocated 1536 bytes for HTTP_Response (Free: 245760 -> 244224)
[00:00:12.890] [DEBUG][WEATHER] HTTP code: 200
[00:00:13.125] [DEBUG][MEMORY] SafeBuffer 'HTTP_Response' destructor - freeing 1536 bytes
[00:00:13.130] [DEBUG][MEMORY] Memory freed (Free: 244224 -> 245760)
[00:00:13.234] [INFO][MEMORY] Memory - Heap: 156234/327680 bytes (47.7%), PSRAM: 0/8388608 bytes (0.0%)
[00:00:15.456] [INFO][WEATHER] Web configuration server started, access via IP address
[00:00:20.789] [INFO][WEATHER] City search request URL: https://devapi.qweather.com/geo/v2/city/lookup?location=北京&number=10
[00:00:22.123] [INFO][WEATHER] City search HTTP response code: 200
```

## 生产环境优化

### 发布模式配置
```cpp
// 在生产环境中，只显示重要信息
Logger::init(LOG_LEVEL_WARN);  // 只显示警告和错误

// 或者完全关闭某些模块的日志
Logger::setModuleLevel(LOG_MODULE_DEBUG, LOG_LEVEL_NONE);
Logger::setModuleLevel(LOG_MODULE_VERBOSE, LOG_LEVEL_NONE);
```

### 内存优化
- 日志系统使用minimal内存占用
- 格式化缓冲区大小优化（512字节）
- 支持日志级别过滤，避免不必要的字符串处理

## 从旧系统迁移

### 替换模式
| 旧代码 | 新代码 |
|--------|--------|
| `Serial.println("错误信息");` | `LOG_SYSTEM_ERROR("错误信息");` |
| `Serial.print("调试: "); Serial.println(value);` | `LOG_DEBUG(LOG_MODULE_SYSTEM, "调试: %d", value);` |
| `Serial.printf("状态: %s\n", status);` | `LOG_INFO(LOG_MODULE_SYSTEM, "状态: %s", status);` |

### 批量更新建议
1. 确定日志所属模块
2. 根据重要性选择日志级别
3. 使用 printf 风格的格式化
4. 移除手动换行符（系统自动处理）

## 性能影响

- **编译时优化**: 不使用的日志级别可以在编译时完全移除
- **运行时过滤**: 高效的级别检查，避免不必要的字符串操作
- **内存友好**: 固定大小的格式化缓冲区
- **线程安全**: 在ESP32多任务环境中安全使用

## 调试技巧

### 临时提高日志级别
```cpp
// 临时调试某个功能时
Logger::setModuleLevel(LOG_MODULE_WIFI, LOG_LEVEL_VERBOSE);
// ... 进行调试 ...
Logger::setModuleLevel(LOG_MODULE_WIFI, LOG_LEVEL_INFO);  // 恢复正常级别
```

### 条件日志
```cpp
if (connectionFailures > 3) {
    LOG_WIFI_ERROR("多次连接失败，可能需要重置配置");
}
```

## 文件结构

```
include/utils/logger.h          # 日志系统头文件，包含所有接口和宏定义
src/utils/logger.cpp           # 日志系统实现文件
```

## 注意事项

1. **初始化顺序**: 确保在使用任何日志功能前调用 `Logger::init()`
2. **格式化安全**: 使用类型安全的 printf 格式化
3. **性能考虑**: 在高频调用的地方考虑使用合适的日志级别
4. **内存限制**: 单条日志消息不应超过512字符

## 未来扩展

- [ ] 支持日志输出到文件系统
- [ ] 网络日志发送功能
- [ ] 日志轮转和存储管理
- [ ] 彩色终端输出支持
- [ ] 远程日志级别动态调整

## 总结

这个统一日志系统显著改善了ESP32天气站项目的调试体验，提供了专业级的日志管理功能，同时保持了高性能和低内存占用。通过模块化的日志级别控制，开发者可以精确地控制调试输出，提高开发效率。