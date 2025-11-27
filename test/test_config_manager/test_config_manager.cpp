#include <unity.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include "lcd_driver.h"
#include "config_manager.h"
#include "../common/test_init.h"  // 引入共享的测试初始化

// 测试辅助类 - 暴露 protected 方法
class TestConfigManager : public ConfigManager {
public:
    TestConfigManager(const String& path) : ConfigManager(path) {}
    
    bool loadConfig() override {}
    bool saveConfig() override {}
    bool resetConfig() override {}
    bool init() override {}
    
    // 暴露 protected 方法供测试使用
    bool publicReadFile(String& content) {
        return readFile(content);
    }
    
    bool publicWriteFile(const String& content) {
        return writeFile(content);
    }
    
    // 暴露 listDir 方法
    static void publicListDir(const char* dirname, uint8_t levels) {
        listDir(dirname, levels);
    }
};

// 在每个测试前执行
void setUp(void) {
    perTestSetup();  // 使用共享的初始化函数
}

// 在每个测试后执行
void tearDown(void) {
    perTestTeardown();  // 使用共享的清理函数
    
    // 清理本测试特定的文件
    if (SPIFFS.exists("/test_config.json")) {
        SPIFFS.remove("/test_config.json");
    }
    if (SPIFFS.exists("/test1.txt")) {
        SPIFFS.remove("/test1.txt");
    }
}

// ============ 测试用例 ============

// 测试0: 失败测试
void test_failure_example(void) {
    TEST_FAIL_MESSAGE("This is an intentional failure for demonstration purposes.");
}

// 测试1: SPIFFS 初始化
void test_spiffs_initialization(void) {
    Serial.println("\n\n----------Starting SPIFFS Initialization Test...----------");
    bool result = ConfigManager::initSPIFFS();
    TEST_ASSERT_TRUE_MESSAGE(result, "SPIFFS should initialize successfully");
    TEST_ASSERT_TRUE_MESSAGE(SPIFFS.begin(), "SPIFFS should be mounted");
}

// 测试2: 写入配置文件
void test_write_config_file(void) {
    Serial.println("\n\n----------Starting Write Config File Test...----------");
    TestConfigManager manager("/test_config.json");
    String testContent = "{\"test\": \"value\", \"number\": 123}";
    
    bool writeResult = manager.publicWriteFile(testContent);
    TEST_ASSERT_TRUE_MESSAGE(writeResult, "Should write config file successfully");
    TEST_ASSERT_TRUE_MESSAGE(SPIFFS.exists("/test_config.json"), "Config file should exist");
}

// 测试3: 读取配置文件
void test_read_config_file(void) {
    Serial.println("\n\n----------Starting Read Config File Test...----------");
    TestConfigManager manager("/test_config.json");
    String originalContent = "{\"key\": \"value\"}";
    
    // 先写入
    manager.publicWriteFile(originalContent);
    
    // 再读取
    String readContent;
    bool readResult = manager.publicReadFile(readContent);
    
    TEST_ASSERT_TRUE_MESSAGE(readResult, "Should read config file successfully");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        originalContent.c_str(), 
        readContent.c_str(),
        "Read content should match written content"
    );
}

// 测试4: 读取不存在的文件
void test_read_nonexistent_file(void) {
    Serial.println("\n\n----------Starting Read Nonexistent File Test...----------");
    TestConfigManager manager("/nonexistent.json");
    String content;
    
    bool result = manager.publicReadFile(content);
    TEST_ASSERT_FALSE_MESSAGE(result, "Should fail to read non-existent file");
}

// 测试5: 写入空内容
void test_write_empty_content(void) {
    Serial.println("\n\n----------Starting Write Empty Content Test...----------");
    TestConfigManager manager("/test_config.json");
    String emptyContent = "";
    
    bool result = manager.publicWriteFile(emptyContent);
    TEST_ASSERT_TRUE_MESSAGE(result, "Should handle empty content");
    
    String readContent;
    manager.publicReadFile(readContent);
    TEST_ASSERT_EQUAL_STRING("", readContent.c_str());
}

// 测试6: 写入大文件
void test_write_large_file(void) {
    Serial.println("\n\n----------Starting Write Large File Test...----------");
    TestConfigManager manager("/test_config.json");
    
    // 创建一个较大的 JSON 字符串
    String largeContent = "{";
    for (int i = 0; i < 100; i++) {
        if (i > 0) largeContent += ",";
        largeContent += "\"key" + String(i) + "\": \"value" + String(i) + "\"";
    }
    largeContent += "}";
    
    bool writeResult = manager.publicWriteFile(largeContent);
    TEST_ASSERT_TRUE_MESSAGE(writeResult, "Should write large file successfully");
    
    String readContent;
    manager.publicReadFile(readContent);
    TEST_ASSERT_EQUAL_STRING(largeContent.c_str(), readContent.c_str());
}

// 测试7: listDir 不崩溃 (修复: 使用 publicListDir)
void test_list_directory_no_crash(void) {
    Serial.println("\n\n----------Starting List Directory No Crash Test...----------");
    // 创建一些测试文件
    File file1 = SPIFFS.open("/test1.txt", "w");
    file1.print("test data");
    file1.close();
    
    // 通过测试辅助类调用 protected 方法
    TestConfigManager::publicListDir("/", 0);
    
    TEST_ASSERT_TRUE_MESSAGE(true, "listDir should not crash");
}

// 测试8: 多次初始化 SPIFFS
void test_multiple_spiffs_init(void) {
    Serial.println("\n\n----------Starting Multiple SPIFFS Init Test...----------");
    bool result1 = ConfigManager::initSPIFFS();
    bool result2 = ConfigManager::initSPIFFS();
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
}

// 测试9: 覆盖现有文件
void test_overwrite_existing_file(void) {
    Serial.println("\n\n----------Starting Overwrite Existing File Test...----------");
    TestConfigManager manager("/test_config.json");
    
    String content1 = "{\"version\": 1}";
    String content2 = "{\"version\": 2}";
    
    manager.publicWriteFile(content1);
    manager.publicWriteFile(content2);
    
    String readContent;
    manager.publicReadFile(readContent);
    
    TEST_ASSERT_EQUAL_STRING(content2.c_str(), readContent.c_str());
}

// 测试10: 特殊字符处理
void test_special_characters(void) {
    Serial.println("\n\n----------Starting Special Characters Test...----------");
    TestConfigManager manager("/test_config.json");
    String specialContent = "{\"text\": \"Hello\\nWorld\\t测试\"}";
    
    manager.publicWriteFile(specialContent);
    
    String readContent;
    manager.publicReadFile(readContent);
    
    TEST_ASSERT_EQUAL_STRING(specialContent.c_str(), readContent.c_str());
}

// ============ Unity 入口 ============

void setup() {
    delay(2000);  // 等待串口稳定
    
    globalTestSetup();  // 全局初始化 - 只在开始时调用一次
    
    UNITY_BEGIN();
    
    lcdText("Running Tests...",1);
    // RUN_TEST(test_failure_example);
    RUN_TEST(test_spiffs_initialization);
    RUN_TEST(test_write_config_file);
    RUN_TEST(test_read_config_file);
    RUN_TEST(test_read_nonexistent_file);
    RUN_TEST(test_write_empty_content);
    RUN_TEST(test_write_large_file);
    RUN_TEST(test_list_directory_no_crash);
    RUN_TEST(test_multiple_spiffs_init);
    RUN_TEST(test_overwrite_existing_file);
    RUN_TEST(test_special_characters);
    lcdText("Tests Completed",1);
    
    UNITY_END();

    tearDown();
    
    globalTestTeardown();  // 全局清理 - 只在结束时调用一次
}

void loop() {
    // 测试完成后不执行任何操作
}