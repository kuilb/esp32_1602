#include <unity.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "lcd_driver.h"
#include "wifi_config_manager.h"
#include "../common/test_init.h"

// ============ 测试前后钩子 ============

void setUp(void) {
    perTestSetup();
}

void tearDown(void) {
    perTestTeardown();
    
    // 清理测试文件
    if (SPIFFS.exists("/test_wifi.json")) {
        SPIFFS.remove("/test_wifi.json");
    }
    if (SPIFFS.exists("/test_wifi_temp.json")) {
        SPIFFS.remove("/test_wifi_temp.json");
    }
}

// ============ 测试用例 ============

// 测试1: WiFi 配置管理器初始化
void test_wifi_manager_initialization(void) {
    Serial.println("\n\n----------Starting WiFi Manager Initialization Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    TEST_ASSERT_TRUE_MESSAGE(
        wifiConfig.init(),
        "WiFi config manager should initialize successfully"
    );
    
    TEST_ASSERT_TRUE_MESSAGE(
        SPIFFS.exists("/test_wifi.json"),
        "Config file should be created after init"
    );
}

// 测试2: 保存 WiFi 配置
void test_save_wifi_config(void) {
    Serial.println("\n\n----------Starting Save WiFi Config Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // 设置配置
    bool setSSIDResult = wifiConfig.setSSID("TestNetwork");
    bool setPasswordResult = wifiConfig.setPassword("TestPassword123");
    
    TEST_ASSERT_TRUE_MESSAGE(setSSIDResult, "Should set SSID successfully");
    TEST_ASSERT_TRUE_MESSAGE(setPasswordResult, "Should set password successfully");
    
    // 验证文件存在
    TEST_ASSERT_TRUE_MESSAGE(
        SPIFFS.exists("/test_wifi.json"),
        "Config file should exist after save"
    );
    
    // 读取并验证文件内容
    File file = SPIFFS.open("/test_wifi.json", "r");
    TEST_ASSERT_TRUE_MESSAGE(file, "Should open config file");
    
    String content = file.readString();
    file.close();
    
    LOG_CONFIG_VERBOSE("Saved content: %s\n", content.c_str());
    
    // 验证 JSON 内容
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    TEST_ASSERT_FALSE_MESSAGE(error, "JSON should be valid");
    
    String savedSSID = doc["ssid"].as<String>();
    String savedPassword = doc["password"].as<String>();
    
    TEST_ASSERT_EQUAL_STRING("TestNetwork", savedSSID.c_str());
    TEST_ASSERT_EQUAL_STRING("TestPassword123", savedPassword.c_str());
}

// 测试3: 加载 WiFi 配置
void test_load_wifi_config(void) {
    Serial.println("\n\n----------Starting Load WiFi Config Test...----------");
    
    // 先保存配置
    WifiConfigManager saveConfig("/test_wifi.json");
    saveConfig.setSSID("LoadTestNetwork");
    saveConfig.setPassword("LoadTestPassword");
    
    // 创建新实例加载配置
    WifiConfigManager loadConfig("/test_wifi.json");
    bool loadResult = loadConfig.loadConfig();
    
    TEST_ASSERT_TRUE_MESSAGE(loadResult, "Should load config successfully");
    
    // 验证加载的数据
    String loadedSSID = loadConfig.getSSID();
    String loadedPassword = loadConfig.getPassword();
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "LoadTestNetwork",
        loadedSSID.c_str(),
        "Loaded SSID should match saved SSID"
    );
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "LoadTestPassword",
        loadedPassword.c_str(),
        "Loaded password should match saved password"
    );
}

// 测试4: 重置 WiFi 配置
void test_reset_wifi_config(void) {
    Serial.println("\n\n----------Starting Reset WiFi Config Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // 先设置自定义配置
    wifiConfig.setSSID("CustomNetwork");
    wifiConfig.setPassword("CustomPassword");
    
    // 重置配置
    bool resetResult = wifiConfig.resetConfig();
    TEST_ASSERT_TRUE_MESSAGE(resetResult, "Should reset config successfully");
    
    // 验证已重置为默认值
    String ssid = wifiConfig.getSSID();
    String password = wifiConfig.getPassword();
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "",
        ssid.c_str(),
        "SSID should be reset to default"
    );
    
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "",
        password.c_str(),
        "Password should be reset to empty"
    );
}

// 测试5: 设置空 SSID（应该失败）
void test_set_empty_ssid(void) {
    Serial.println("\n\n----------Starting Set Empty SSID Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    bool result = wifiConfig.setSSID("");
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should reject empty SSID"
    );
}

// 测试6: 设置过长 SSID（应该失败）
void test_set_too_long_ssid(void) {
    Serial.println("\n\n----------Starting Set Too Long SSID Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // WiFi SSID 最大长度为 32 字符
    String longSSID = "ThisIsAVeryLongSSIDThatExceeds32Characters!!!";
    bool result = wifiConfig.setSSID(longSSID);
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should reject SSID longer than 32 characters"
    );
}

// 测试7: 设置有效 SSID
void test_set_valid_ssid(void) {
    Serial.println("\n\n----------Starting Set Valid SSID Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    bool result = wifiConfig.setSSID("ValidSSID");
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Should accept valid SSID");
    TEST_ASSERT_EQUAL_STRING("ValidSSID", wifiConfig.getSSID().c_str());
}

// 测试8: 设置短密码（应该失败）
void test_set_short_password(void) {
    Serial.println("\n\n----------Starting Set Short Password Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // WPA 密码最少 8 个字符
    bool result = wifiConfig.setPassword("1234567");  // 只有 7 个字符
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should reject password shorter than 8 characters"
    );
}

// 测试9: 设置过长密码（应该失败）
void test_set_too_long_password(void) {
    Serial.println("\n\n----------Starting Set Too Long Password Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // WiFi 密码最大长度为 63 字符
    String longPassword = "ThisPasswordIsWayTooLongForWiFiStandardsAndShouldBeRejectedByValidationLogic";
    bool result = wifiConfig.setPassword(longPassword);
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should reject password longer than 63 characters"
    );
}

// 测试10: 设置有效密码
void test_set_valid_password(void) {
    Serial.println("\n\n----------Starting Set Valid Password Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    bool result = wifiConfig.setPassword("ValidPassword123");
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Should accept valid password");
    TEST_ASSERT_EQUAL_STRING("ValidPassword123", wifiConfig.getPassword().c_str());
}

// 测试11: 设置空密码（开放网络）
void test_set_empty_password(void) {
    Serial.println("\n\n----------Starting Set Empty Password Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    bool result = wifiConfig.setPassword("");
    
    TEST_ASSERT_TRUE_MESSAGE(
        result,
        "Should accept empty password for open network"
    );
    
    TEST_ASSERT_EQUAL_STRING("", wifiConfig.getPassword().c_str());
}

// 测试12: 加载不存在的配置文件
void test_load_nonexistent_config(void) {
    Serial.println("\n\n----------Starting Load Nonexistent Config Test...----------");
    
    // 确保文件不存在
    if (SPIFFS.exists("/nonexistent.json")) {
        SPIFFS.remove("/nonexistent.json");
    }
    
    WifiConfigManager wifiConfig("/nonexistent.json");
    bool result = wifiConfig.loadConfig();
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should fail to load non-existent config file"
    );
}

// 测试13: 加载损坏的 JSON 文件
void test_load_corrupted_json(void) {
    Serial.println("\n\n----------Starting Load Corrupted JSON Test...----------");
    
    // 写入无效的 JSON
    File file = SPIFFS.open("/test_wifi.json", "w");
    file.print("{invalid json content}");
    file.close();
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    bool result = wifiConfig.loadConfig();
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should fail to load corrupted JSON"
    );
}

// 测试14: 多次保存和加载
void test_multiple_save_load_cycles(void) {
    Serial.println("\n\n----------Starting Multiple Save/Load Cycles Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // 循环 5 次保存和加载
    for (int i = 1; i <= 5; i++) {
        String ssid = "Network" + String(i);
        String password = "Password" + String(i);
        
        // 保存
        wifiConfig.setSSID(ssid);
        wifiConfig.setPassword(password);
        
        // 重新加载
        WifiConfigManager tempConfig("/test_wifi.json");
        tempConfig.loadConfig();
        
        // 验证
        TEST_ASSERT_EQUAL_STRING(ssid.c_str(), tempConfig.getSSID().c_str());
        TEST_ASSERT_EQUAL_STRING(password.c_str(), tempConfig.getPassword().c_str());
    }
}

// 测试15: Getter 方法
void test_getter_methods(void) {
    Serial.println("\n\n----------Starting Getter Methods Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    wifiConfig.setSSID("TestSSID");
    wifiConfig.setPassword("TestPassword");
    
    String ssid = wifiConfig.getSSID();
    String password = wifiConfig.getPassword();
    
    TEST_ASSERT_EQUAL_STRING("TestSSID", ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("TestPassword", password.c_str());
}

// 测试16: JSON 格式验证
void test_json_format_validation(void) {
    Serial.println("\n\n----------Starting JSON Format Validation Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    wifiConfig.setSSID("FormatTest");
    wifiConfig.setPassword("FormatPassword");
    
    // 读取保存的 JSON
    File file = SPIFFS.open("/test_wifi.json", "r");
    String content = file.readString();
    file.close();
    
    Serial.printf("JSON content: %s\n", content.c_str());
    
    // 验证 JSON 格式
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    
    TEST_ASSERT_FALSE_MESSAGE(error, "JSON should be valid");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("ssid"), "JSON should contain 'ssid' key");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("password"), "JSON should contain 'password' key");
}

// 测试17: 特殊字符处理
void test_special_characters_in_config(void) {
    Serial.println("\n\n----------Starting Special Characters Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // 包含特殊字符的 SSID 和密码
    String specialSSID = "Test-WiFi_2.4G";
    String specialPassword = "P@ssw0rd!#$%";
    
    wifiConfig.setSSID(specialSSID);
    wifiConfig.setPassword(specialPassword);
    
    // 重新加载
    WifiConfigManager loadConfig("/test_wifi.json");
    loadConfig.loadConfig();
    
    TEST_ASSERT_EQUAL_STRING(specialSSID.c_str(), loadConfig.getSSID().c_str());
    TEST_ASSERT_EQUAL_STRING(specialPassword.c_str(), loadConfig.getPassword().c_str());
}

// 测试18: Unicode 字符处理
void test_unicode_characters(void) {
    Serial.println("\n\n----------Starting Unicode Characters Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    // 包含中文字符
    String unicodeSSID = "测试WiFi";
    String unicodePassword = "密码123";
    
    wifiConfig.setSSID(unicodeSSID);
    wifiConfig.setPassword(unicodePassword);
    
    // 重新加载
    WifiConfigManager loadConfig("/test_wifi.json");
    loadConfig.loadConfig();
    
    TEST_ASSERT_EQUAL_STRING(unicodeSSID.c_str(), loadConfig.getSSID().c_str());
    TEST_ASSERT_EQUAL_STRING(unicodePassword.c_str(), loadConfig.getPassword().c_str());
}

// 测试19: 并发创建多个配置实例
void test_multiple_config_instances(void) {
    Serial.println("\n\n----------Starting Multiple Config Instances Test...----------");
    
    WifiConfigManager config1("/test_wifi.json");
    WifiConfigManager config2("/test_wifi_temp.json");
    
    config1.setSSID("Network1");
    config1.setPassword("Password1");
    
    config2.setSSID("Network2");
    config2.setPassword("Password2");
    
    // 验证两个实例互不影响
    TEST_ASSERT_EQUAL_STRING("Network1", config1.getSSID().c_str());
    TEST_ASSERT_EQUAL_STRING("Network2", config2.getSSID().c_str());
}

// 测试20: 文件大小验证
void test_config_file_size(void) {
    Serial.println("\n\n----------Starting Config File Size Test...----------");
    
    WifiConfigManager wifiConfig("/test_wifi.json");
    
    wifiConfig.setSSID("TestNetwork");
    wifiConfig.setPassword("TestPassword123");
    
    File file = SPIFFS.open("/test_wifi.json", "r");
    size_t fileSize = file.size();
    file.close();
    
    Serial.printf("Config file size: %d bytes\n", fileSize);
    
    TEST_ASSERT_TRUE_MESSAGE(
        fileSize > 0 && fileSize < 1024,
        "Config file size should be reasonable (0-1024 bytes)"
    );
}

// ============ Unity 主函数 ============

void setup() {
    delay(2000);  // 等待串口稳定
    
    globalTestSetup();  // 全局初始化
    
    UNITY_BEGIN();
    
    lcdText("WiFi Config Tests", 1);
    
    // 基础功能测试
    RUN_TEST(test_wifi_manager_initialization);
    RUN_TEST(test_save_wifi_config);
    RUN_TEST(test_load_wifi_config);
    RUN_TEST(test_reset_wifi_config);
    
    // SSID 验证测试
    RUN_TEST(test_set_empty_ssid);
    RUN_TEST(test_set_too_long_ssid);
    RUN_TEST(test_set_valid_ssid);
    
    // 密码验证测试
    RUN_TEST(test_set_short_password);
    RUN_TEST(test_set_too_long_password);
    RUN_TEST(test_set_valid_password);
    RUN_TEST(test_set_empty_password);
    
    // 错误处理测试
    RUN_TEST(test_load_nonexistent_config);
    RUN_TEST(test_load_corrupted_json);
    
    // 高级功能测试
    RUN_TEST(test_multiple_save_load_cycles);
    RUN_TEST(test_getter_methods);
    RUN_TEST(test_json_format_validation);
    RUN_TEST(test_special_characters_in_config);
    RUN_TEST(test_unicode_characters);
    RUN_TEST(test_multiple_config_instances);
    RUN_TEST(test_config_file_size);
    
    lcdText("Tests Completed", 1);
    
    UNITY_END();
    
    globalTestTeardown();  // 全局清理
}

void loop() {
    // 测试完成后不执行任何操作
}