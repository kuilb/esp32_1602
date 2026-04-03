# ESP32-1602 Java 上位机 SDK（picolink）

源码目录：`1602api/picolink/`（包名：`picolink`）

入口类：`picolink.PicoLink`

## 编译（Windows）

由于文件含中文注释，建议显式指定 UTF-8 编码：

```bat
cd /d c:\source_code\esp32\esp32_1602
javac -encoding UTF-8 .\1602api\picolink\*.java
```

## 运行示例

```bat
cd /d c:\source_code\esp32\esp32_1602
java -cp .\1602api picolink.ExampleMain <host> <port>
```

示例会：连接设备、开启心跳、发送两行文本。
