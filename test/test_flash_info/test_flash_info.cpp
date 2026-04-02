/**
 * @file test_flash_info.cpp
 * @brief 检测 Flash 芯片详细信息
 * 
 * 运行此测试以获取 Flash 芯片的制造商、型号等信息
 */

#include <Arduino.h>
#include <unity.h>
#include "esp_flash.h"
#include "esp_partition.h"
#include <SPIFFS.h>

void setUp(void) {
    // 在每个测试前执行
}

void tearDown(void) {
    // 在每个测试后执行
}

/**
 * @brief 测试获取 Flash 芯片基本信息
 */
void test_flash_basic_info(void) {
    esp_flash_t* flash = esp_flash_default_chip;
    
    uint32_t flash_id = 0;
    esp_err_t err = esp_flash_read_id(flash, &flash_id);
    
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    Serial.println("\n========== Flash 芯片信息 ==========");
    Serial.printf("Flash ID: 0x%08X\n", flash_id);
    
    // 解析 Flash ID
    uint8_t manufacturer_id = (flash_id >> 16) & 0xFF;
    uint8_t memory_type = (flash_id >> 8) & 0xFF;
    uint8_t capacity = flash_id & 0xFF;
    
    Serial.printf("制造商 ID: 0x%02X ", manufacturer_id);
    
    // 识别制造商
    switch (manufacturer_id) {
        case 0xEF: Serial.println("(Winbond)"); break;
        case 0xC8: Serial.println("(GigaDevice)"); break;
        case 0x20: Serial.println("(XMC)"); break;
        case 0x68: Serial.println("(BOYA)"); break;
        case 0xA1: Serial.println("(Fudan Micro)"); break;
        case 0x5E: Serial.println("(Zbit)"); break;
        case 0x0B: Serial.println("(XTX)"); break;
        case 0xC2: Serial.println("(MXIC)"); break;
        case 0x1C: Serial.println("(EON)"); break;
        case 0xBF: Serial.println("(SST)"); break;
        default: Serial.println("(Unknown)"); break;
    }
    
    Serial.printf("存储类型: 0x%02X\n", memory_type);
    Serial.printf("容量代码: 0x%02X ", capacity);
    
    // 计算实际容量
    if (capacity >= 0x14 && capacity <= 0x19) {
        uint32_t size_bytes = 1 << capacity;
        Serial.printf("(~%d MB)\n", size_bytes / (1024 * 1024));
    } else {
        Serial.println("(Unknown)");
    }
    
    // 获取 Flash 大小
    uint32_t flash_size = 0;
    err = esp_flash_get_size(flash, &flash_size);
    if (err == ESP_OK) {
        Serial.printf("Flash 总大小: %u bytes (%u MB)\n", 
                     flash_size, flash_size / (1024 * 1024));
    }
    
    Serial.println("====================================\n");
}

/**
 * @brief 测试 Flash 写入和读取稳定性
 */
void test_flash_write_read(void) {
    Serial.println("\n========== Flash 写入读取测试 ==========");
    
    // 初始化 SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS 初始化失败!");
        TEST_FAIL_MESSAGE("SPIFFS initialization failed");
        return;
    }
    
    Serial.println("SPIFFS 初始化成功");
    Serial.printf("总空间: %u bytes, 已用: %u bytes\n", 
                 SPIFFS.totalBytes(), SPIFFS.usedBytes());
    
    // 测试数据
    const char* test_file = "/flash_test.txt";
    const char* test_data = "ESP32-S3 Flash Write/Read Test 1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t data_len = strlen(test_data);
    
    // === 测试 1: 基本写入读取 ===
    Serial.println("\n[测试 1] 基本写入读取:");
    
    // 删除旧测试文件
    if (SPIFFS.exists(test_file)) {
        SPIFFS.remove(test_file);
    }
    
    // 写入
    uint32_t write_start = micros();
    File file = SPIFFS.open(test_file, "w");
    TEST_ASSERT_TRUE_MESSAGE(file, "无法打开文件进行写入");
    
    size_t written = file.print(test_data);
    file.flush();
    file.close();
    uint32_t write_time = micros() - write_start;
    
    Serial.printf("  写入 %u 字节, 耗时 %u us\n", written, write_time);
    TEST_ASSERT_EQUAL_MESSAGE(data_len, written, "写入字节数不匹配");
    
    // 读取
    uint32_t read_start = micros();
    file = SPIFFS.open(test_file, "r");
    TEST_ASSERT_TRUE_MESSAGE(file, "无法打开文件进行读取");
    
    String read_data = file.readString();
    file.close();
    uint32_t read_time = micros() - read_start;
    
    Serial.printf("  读取 %u 字节, 耗时 %u us\n", read_data.length(), read_time);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(test_data, read_data.c_str(), "读取数据不匹配");
    
    Serial.println("  ✓ 基本读写测试通过");
    
    // === 测试 2: 多次写入测试稳定性 ===
    Serial.println("\n[测试 2] 多次写入稳定性 (10次):");
    
    int success_count = 0;
    int fail_count = 0;
    uint32_t total_write_time = 0;
    
    for (int i = 0; i < 10; i++) {
        char test_content[128];
        snprintf(test_content, sizeof(test_content), 
                "Test iteration %d: %s", i, test_data);
        
        // 删除旧文件
        if (SPIFFS.exists(test_file)) {
            SPIFFS.remove(test_file);
            delay(10);
        }
        
        // 写入
        write_start = micros();
        file = SPIFFS.open(test_file, "w");
        if (!file) {
            Serial.printf("  [%d] 写入失败: 无法打开文件\n", i);
            fail_count++;
            continue;
        }
        
        written = file.print(test_content);
        file.flush();
        file.close();
        write_time = micros() - write_start;
        total_write_time += write_time;
        
        delay(10);
        
        // 读取验证
        file = SPIFFS.open(test_file, "r");
        if (!file) {
            Serial.printf("  [%d] 读取失败: 无法打开文件\n", i);
            fail_count++;
            continue;
        }
        
        read_data = file.readString();
        file.close();
        
        if (read_data.equals(test_content)) {
            success_count++;
        } else {
            Serial.printf("  [%d] 数据不匹配!\n", i);
            Serial.printf("      期望: %s\n", test_content);
            Serial.printf("      实际: %s\n", read_data.c_str());
            fail_count++;
        }
    }
    
    Serial.printf("  成功: %d/10, 失败: %d/10\n", success_count, fail_count);
    Serial.printf("  平均写入时间: %u us\n", total_write_time / 10);
    
    TEST_ASSERT_EQUAL_MESSAGE(10, success_count, "多次写入测试有失败");
    Serial.println("  ✓ 多次写入测试通过");
    
    // === 测试 3: 大文件写入读取 ===
    Serial.println("\n[测试 3] 大文件写入 (16KB):");
    
    const size_t large_size = 16384;  // 16KB
    char* large_data = (char*)malloc(large_size + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(large_data, "无法分配内存");
    
    // 生成测试数据
    for (size_t i = 0; i < large_size; i++) {
        large_data[i] = 'A' + (i % 26);
    }
    large_data[large_size] = '\0';
    
    // 删除旧文件
    if (SPIFFS.exists(test_file)) {
        SPIFFS.remove(test_file);
        delay(50);
    }
    
    // 写入大文件
    write_start = micros();
    file = SPIFFS.open(test_file, "w");
    TEST_ASSERT_TRUE_MESSAGE(file, "无法打开大文件进行写入");
    
    written = file.write((uint8_t*)large_data, large_size);
    file.flush();
    file.close();
    write_time = micros() - write_start;
    
    Serial.printf("  写入 %u 字节, 耗时 %u us (%.2f KB/s)\n", 
                 written, write_time, 
                 (written / 1024.0) / (write_time / 1000000.0));
    TEST_ASSERT_EQUAL_MESSAGE(large_size, written, "大文件写入字节数不匹配");
    
    delay(100);
    
    // 读取大文件
    read_start = micros();
    file = SPIFFS.open(test_file, "r");
    TEST_ASSERT_TRUE_MESSAGE(file, "无法打开大文件进行读取");
    
    size_t file_size = file.size();
    Serial.printf("  文件大小: %u bytes\n", file_size);
    TEST_ASSERT_EQUAL_MESSAGE(large_size, file_size, "文件大小不匹配");
    
    char* read_large = (char*)malloc(large_size + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(read_large, "无法分配读取缓冲区");
    
    size_t bytes_read = file.readBytes(read_large, large_size);
    file.close();
    read_time = micros() - read_start;
    
    Serial.printf("  读取 %u 字节, 耗时 %u us (%.2f KB/s)\n", 
                 bytes_read, read_time,
                 (bytes_read / 1024.0) / (read_time / 1000000.0));
    
    // 验证数据
    bool data_match = (memcmp(large_data, read_large, large_size) == 0);
    TEST_ASSERT_TRUE_MESSAGE(data_match, "大文件数据验证失败");
    
    Serial.println("  ✓ 大文件读写测试通过");
    
    free(large_data);
    free(read_large);
    
    // 清理
    SPIFFS.remove(test_file);
    
    Serial.println("\n====================================");
    Serial.println("所有 Flash 读写测试通过! ✓");
    Serial.println("====================================\n");
}

/**
 * @brief 测试分区信息
 */
void test_partition_info(void) {
    Serial.println("\n========== Flash 分区信息 ==========");
    
    esp_partition_iterator_t it = esp_partition_find(
        ESP_PARTITION_TYPE_ANY, 
        ESP_PARTITION_SUBTYPE_ANY, 
        NULL
    );
    
    while (it != NULL) {
        const esp_partition_t* partition = esp_partition_get(it);
        
        Serial.printf("分区: %-16s ", partition->label);
        Serial.printf("类型: 0x%02x ", partition->type);
        Serial.printf("子类型: 0x%02x ", partition->subtype);
        Serial.printf("地址: 0x%08X ", partition->address);
        Serial.printf("大小: %u KB\n", partition->size / 1024);
        
        it = esp_partition_next(it);
    }
    
    esp_partition_iterator_release(it);
    
    Serial.println("====================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // 等待串口稳定
    
    Serial.println("\n\n========== Flash 芯片检测开始 ==========\n");
    
    UNITY_BEGIN();
    
    RUN_TEST(test_flash_basic_info);
    RUN_TEST(test_partition_info);
    RUN_TEST(test_flash_write_read);  // 新增的写入读取测试
    
    UNITY_END();
    
    Serial.println("\n========== 检测完成 ==========\n");
}

void loop() {
    // 测试只运行一次
    delay(1000);
}
