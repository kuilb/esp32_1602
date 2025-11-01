# 内存管理模块日志示例

## 内存管理模块日志输出示例

### 正常内存分配流程
```
[00:00:05.120] [DEBUG][MEMORY] Allocated 1024 bytes for HTTP_Response (Free: 245760 -> 244736)
[00:00:05.125] [DEBUG][MEMORY] Allocated 512 bytes for JSON_Buffer (Free: 244736 -> 244224)
[00:00:05.890] [DEBUG][MEMORY] Memory freed (Free: 244224 -> 244736)
[00:00:05.895] [DEBUG][MEMORY] SafeBuffer 'HTTP_Response' destructor - freeing 1024 bytes
[00:00:05.900] [DEBUG][MEMORY] Memory freed (Free: 244736 -> 245760)
```

### 内存使用情况监控
```
[00:00:04.100] [INFO][MEMORY] Weather fetch start - Free: 245760 bytes, Max Alloc: 110592 bytes, Min Free: 234568 bytes
[00:00:08.345] [INFO][MEMORY] After HTTP request - Free: 244736 bytes, Max Alloc: 110592 bytes, Min Free: 234568 bytes
[00:00:12.670] [INFO][MEMORY] JSON parsing complete - Free: 245760 bytes, Max Alloc: 110592 bytes, Min Free: 234568 bytes
```

### 内存分配警告示例
```
[00:00:06.234] [WARN][MEMORY] Attempting to allocate 0 bytes for EmptyBuffer
[00:00:06.567] [WARN][MEMORY] Large allocation 98304 bytes for LargeBuffer (40.0% of free heap)
```

### 内存分配错误示例
```
[00:00:07.123] [ERROR][MEMORY] Request 131072 bytes for HugeBuffer, but max allocable is 110592
[00:00:07.456] [ERROR][MEMORY] Failed to allocate 8192 bytes for DataBuffer
[00:00:07.461] [ERROR][MEMORY] Available: Free=4096, MaxAlloc=3584
```

### SafeBuffer RAII 自动管理
```
[00:00:05.120] [DEBUG][MEMORY] Allocated 2048 bytes for WeatherData (Free: 245760 -> 243712)
[00:00:05.780] [DEBUG][MEMORY] SafeBuffer 'WeatherData' destructor - freeing 2048 bytes
[00:00:05.785] [DEBUG][MEMORY] Memory freed (Free: 243712 -> 245760)
```

### 内存碎片化检测
```
[00:00:08.123] [INFO][MEMORY] System startup - Free: 245760 bytes, Max Alloc: 110592 bytes, Min Free: 245760 bytes
[00:00:15.456] [INFO][MEMORY] After operations - Free: 234568 bytes, Max Alloc: 89216 bytes, Min Free: 198432 bytes
[00:00:15.461] [WARN][MEMORY] Large allocation 65536 bytes for ImageBuffer (28.0% of free heap)
```

## 日志级别详解

### ERROR级别 - 严重内存问题
- **分配失败**: 无法分配所需内存
- **超出限制**: 请求大小超过最大可分配块
- **系统状态**: 显示当前可用内存状态

### WARN级别 - 需要注意的情况
- **零字节分配**: 尝试分配0字节（可能的程序逻辑错误）
- **大内存分配**: 超过80%剩余内存的分配请求

### INFO级别 - 重要内存状态信息
- **内存使用概览**: 通过`printMemoryInfo()`显示系统内存状态
- **关键节点监控**: 在重要操作前后记录内存状态

### DEBUG级别 - 详细分配追踪
- **成功分配**: 记录分配大小、用途、内存变化
- **内存释放**: 记录释放操作和内存恢复情况
- **RAII析构**: SafeBuffer自动析构时的详细信息

## 内存监控最佳实践

### 1. 关键操作前后监控
```cpp
MemoryManager::printMemoryInfo("Before large operation");
// 执行大内存操作
MemoryManager::printMemoryInfo("After large operation");
```

### 2. 使用RAII自动管理
```cpp
{
    MemoryManager::SafeBuffer buffer(1024, "TempData");
    // 使用buffer...
    // 作用域结束时自动释放，有DEBUG日志
}
```

### 3. 生产环境配置
```cpp
// 只显示重要的内存问题
Logger::setModuleLevel(LOG_MODULE_MEMORY, LOG_LEVEL_WARN);
```

### 4. 开发调试配置
```cpp
// 显示详细的内存分配追踪
Logger::setModuleLevel(LOG_MODULE_MEMORY, LOG_LEVEL_DEBUG);
```

## 便捷宏使用

```cpp
// 基本内存日志
LOG_MEMORY_ERROR("Critical memory issue: %s", error_msg);
LOG_MEMORY_WARN("Memory usage high: %.1f%%", usage_percent);
LOG_MEMORY_DEBUG("Allocated %u bytes for %s", size, name);

// 内存状态快照
LOG_MEMORY_INFO();  // 调用Logger::logMemoryInfo()

// 带上下文的内存信息
LOG_MEMORY_INFO_MSG("At checkpoint: Free=%u, Used=%u", free, used);
```

## 内存泄漏检测

通过日志可以检测潜在的内存泄漏：

```
[00:00:05.120] [DEBUG][MEMORY] Allocated 1024 bytes for Buffer1 (Free: 245760 -> 244736)
[00:00:05.125] [DEBUG][MEMORY] Allocated 2048 bytes for Buffer2 (Free: 244736 -> 242688)
// ... 缺少对应的释放日志可能表示内存泄漏
[00:00:10.000] [INFO][MEMORY] Status check - Free: 242688 bytes (未恢复到初始状态)
```

## 性能分析

内存日志可以帮助分析：
- **分配热点**: 哪些操作频繁分配内存
- **内存使用峰值**: 系统运行过程中的最大内存使用
- **碎片化程度**: MaxAlloc与Free的比值变化
- **RAII效果**: 自动内存管理的执行情况

## 与其他模块的协作

内存管理日志与其他模块日志配合，提供完整的系统视图：

```
[00:00:12.890] [INFO][WEATHER] API config OK, fetching weather...
[00:00:12.895] [INFO][MEMORY] Weather fetch start - Free: 245760 bytes, Max Alloc: 110592 bytes, Min Free: 234568 bytes
[00:00:12.900] [DEBUG][MEMORY] Allocated 1536 bytes for HTTP_Response (Free: 245760 -> 244224)
[00:00:13.234] [DEBUG][WEATHER] HTTP code: 200
[00:00:13.890] [DEBUG][MEMORY] SafeBuffer 'HTTP_Response' destructor - freeing 1536 bytes
[00:00:13.895] [DEBUG][MEMORY] Memory freed (Free: 244224 -> 245760)
[00:00:13.900] [INFO][WEATHER] Weather data updated successfully
```

这样可以清楚地看到每个功能模块的内存使用模式和影响。