# I2C 地址扫描器 (I2C Address Scanner)

这是一个 I2C 总线诊断工具，用来检测并列出所有连接到 I2C 总线上的设备。

## 功能

- 扫描 I2C 地址范围 0x03 到 0x77
- 自动识别已知设备（BQ27421@0x55、OPT3001@0x44）
- 启动后立即执行一次扫描
- 之后每 5 秒自动重复扫描一次
- 结果同时输出到串口和日志系统

## 编译和上传

### 方式 1：使用 PlatformIO 命令行

```bash
# 编译适用于 esp32s3-1602-test 环境
pio run -e esp32s3-1602-test

# 上传到 COM3
pio run -e esp32s3-1602-test --target upload --upload-port COM3

# 监视串口输出
pio device monitor -b 115200 -p COM3
```

### 方式 2：使用 VS Code PlatformIO 插件

1. 打开命令面板 (Ctrl+Shift+P)
2. 输入 "PlatformIO: Build" 并选择
3. 选择环境 "esp32s3-1602-test"
4. 构建完成后，使用 "PlatformIO: Upload" 上传
5. 使用 "PlatformIO: Monitor" 查看串口输出

## 预期输出示例

如果 BQ27421 和 OPT3001 都正常连接：

```
========== I2C Address Scanner ==========
Scanning I2C addresses 0x03 to 0x77...

[I2C] Device found at address: 0x44 (68)
[I2C] Device found at address: 0x55 (85)

========== Scan Results ==========
Total devices found: 2

Devices found at addresses:
  [1] 0x44 - OPT3001 (Light Sensor)
  [2] 0x55 - BQ27421 (Fuel Gauge)
=====================================
```

如果只有 OPT3001 存在（说明 BQ27421 有问题）：

```
========== Scan Results ==========
Total devices found: 1

Devices found at addresses:
  [1] 0x44 - OPT3001 (Light Sensor)
```

## 诊断步骤

1. **都找不到设备**
   - 检查 I2C 上拉电阻（通常为 4.7kΩ to GND）
   - 检查 SDA/SCL 连线是否正确接到 GPIO38/GPIO21
   - 检查器件电源是否接上

2. **只找不到 BQ27421 (0x55)**
   - BQ27421 供电问题或器件故障
   - 检查芯片焊接是否有虚焊

3. **找不到 OPT3001 (0x44)**
   - OPT3001 供电问题或器件故障
   - 检查 ADDR 引脚接地情况

4. **都找到了但主程序报错**
   - 可能是器件初始化或通信参数问题
   - 检查寄存器读写代码

## 相关文件

- 配置定义：`include/mydefine.h`（SDA_PIN、SCL_PIN）
- 电量计驱动：`src/hardware/fuel_gauge.cpp`
- 光传感器驱动：`src/hardware/opt3001.cpp`
