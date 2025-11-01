# JWT认证模块日志示例

## JWT模块日志输出示例

### 正常初始化流程
```
[00:00:04.120] [INFO][JWT] JWT 配置已加载
[00:00:04.125] [DEBUG][JWT] apiHost: api.qweather.com
[00:00:04.130] [DEBUG][JWT] kid: HE1234567890
[00:00:04.135] [DEBUG][JWT] project_id: S1234567890
[00:00:04.140] [DEBUG][JWT] base64_key: MC4CAQAwBQYDK2VwBCIEI...
[00:00:04.145] [DEBUG][JWT] locationID: 101010100
[00:00:04.150] [DEBUG][JWT] city_name: 北京
```

### 错误处理示例
```
[00:00:03.890] [ERROR][JWT] SPIFFS 初始化失败
```
或
```
[00:00:03.890] [WARN][JWT] JWT 配置文件不存在
```
或
```
[00:00:03.890] [ERROR][JWT] 打开 JWT 配置文件失败
```

### Seed32生成过程
```
[00:00:04.200] [DEBUG][JWT] Generating seed32 from base64 key...
[00:00:04.205] [DEBUG][JWT] Extracting Ed25519 seed from DER data...
[00:00:04.210] [DEBUG][JWT] DER length: 46
[00:00:04.215] [DEBUG][JWT] Found Ed25519 seed pattern at offset: 12
[00:00:04.220] [DEBUG][JWT] Successfully extracted 32-byte seed
FA8B9C3E7F2A1B4D9E8C7F6A3B2E1D4C8F7A2B9E3C6D8F1A4B7E9C2D5F8A1B4E
[00:00:04.225] [INFO][JWT] Seed32 generation completed successfully
```

### Seed32生成错误
```
[00:00:04.200] [DEBUG][JWT] Generating seed32 from base64 key...
[00:00:04.205] [ERROR][JWT] No base64 key, cannot generate seed32
```
或
```
[00:00:04.210] [ERROR][JWT] 无法找到seed32! DER bytes:
30 2E 02 01 00 30 05 06 03 2B 65 70 04 22 04 20
[00:00:04.215] [ERROR][JWT] Failed to extract seed32!
```

### JWT令牌生成过程
```
[00:00:10.340] [DEBUG][JWT] Header: {"alg":"EdDSA","kid":"HE1234567890"}
[00:00:10.345] [DEBUG][JWT] 系统时间正常, 时间戳: 1737012745
[00:00:10.350] [DEBUG][JWT] 当前时间: 2025-01-15 14:32:25
[00:00:10.355] [INFO][JWT] JWT token generated successfully, length: 284
```

### 时间同步错误
```
[00:00:10.340] [DEBUG][JWT] Header: {"alg":"EdDSA","kid":"HE1234567890"}
[00:00:10.345] [ERROR][JWT] 系统时间未同步! 当前时间戳: 946684800
[00:00:10.350] [WARN][JWT] 请确保NTP时间同步正常
[00:00:10.355] [INFO][JWT] JWT token generated successfully, length: 284
```

## 日志级别说明

### ERROR级别
- SPIFFS初始化失败
- 无法打开配置文件
- Base64密钥解析失败
- Seed32提取失败
- 系统时间未同步

### WARN级别
- 配置文件不存在
- 时间同步提醒

### INFO级别
- 配置加载成功
- Seed32生成完成
- JWT令牌生成成功

### DEBUG级别
- 配置详细信息（密钥、ID等）
- DER解析过程
- 时间戳和时间信息
- JWT Header信息

## 调试建议

### 生产环境配置
```cpp
// 只显示重要信息
Logger::setModuleLevel(LOG_MODULE_JWT, LOG_LEVEL_INFO);
```

### 开发调试配置
```cpp
// 显示详细调试信息
Logger::setModuleLevel(LOG_MODULE_JWT, LOG_LEVEL_DEBUG);
```

### 安全考虑
- DEBUG级别会显示部分敏感信息（密钥前缀、配置详情）
- 生产环境建议使用INFO级别或更高
- Seed32的完整输出仍使用Serial.printf保持原有调试功能

## 与其他模块的配合

JWT模块的日志与其他模块协同工作：

```
[00:00:03.890] [INFO][SYSTEM] SPIFFS已挂载
[00:00:04.120] [INFO][JWT] JWT 配置已加载
[00:00:06.340] [INFO][WIFI] 正在连接: MyWiFi
[00:00:08.123] [INFO][TIME_SYNC] NTP Time Synced successfully!
[00:00:10.340] [INFO][JWT] JWT token generated successfully, length: 284
[00:00:12.890] [INFO][WEATHER] API config OK, fetching weather...
```

这样可以清晰地看到整个系统的初始化和运行流程。