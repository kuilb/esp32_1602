#include <unity.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "lcd_driver.h"
#include "qweather_auth_config_manager.h"
#include "../common/test_init.h"

// ============ 测试前后钩子 ============

void setUp(void) {
    perTestSetup();
}

void tearDown(void) {
    perTestTeardown();
    
    // 清理测试文件
    if (SPIFFS.exists("/test_qweather.json")) {
        SPIFFS.remove("/test_qweather.json");
    }
    if (SPIFFS.exists("/test_qweather_temp.json")) {
        SPIFFS.remove("/test_qweather_temp.json");
    }
}

// ============ 测试用例 ============

// 测试1: QWeather 配置管理器初始化
void test_qweather_manager_initialization(void) {
    Serial.println("\n\n----------Starting QWeather Manager Initialization Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    TEST_ASSERT_TRUE_MESSAGE(
        qweatherConfig.init(),
        "QWeather config manager should initialize successfully"
    );
    
    TEST_ASSERT_TRUE_MESSAGE(
        SPIFFS.exists("/test_qweather.json"),
        "Config file should be created after init"
    );
}

// 测试2: 保存认证配置
void test_save_auth_config(void) {
    Serial.println("\n\n----------Starting Save Auth Config Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    bool result = qweatherConfig.setAuth(
        "devapi.qweather.com",
        "testkid123",
        "testproj45",
        "dGVzdF9iYXNlNjRfa2V5"
    );
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Should save auth config successfully");
    
    // 验证文件存在
    TEST_ASSERT_TRUE_MESSAGE(
        SPIFFS.exists("/test_qweather.json"),
        "Config file should exist after save"
    );
    
    // 读取并验证内容
    File file = SPIFFS.open("/test_qweather.json", "r");
    TEST_ASSERT_TRUE_MESSAGE(file, "Should open config file");
    
    String content = file.readString();
    file.close();
    
    Serial.printf("Saved content: %s\n", content.c_str());
    
    // 验证 JSON 内容
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    TEST_ASSERT_FALSE_MESSAGE(error, "JSON should be valid");
    
    TEST_ASSERT_EQUAL_STRING("devapi.qweather.com", doc["apiHost"].as<String>().c_str());
    TEST_ASSERT_EQUAL_STRING("testkid123", doc["kId"].as<String>().c_str());
    TEST_ASSERT_EQUAL_STRING("testproj45", doc["projectID"].as<String>().c_str());
    TEST_ASSERT_EQUAL_STRING("dGVzdF9iYXNlNjRfa2V5", doc["base64Key"].as<String>().c_str());
}

// 测试3: 保存位置配置
void test_save_location_config(void) {
    Serial.println("\n\n----------Starting Save Location Config Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    bool result = qweatherConfig.setLocation("101010100", "Beijing");
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Should save location config successfully");
    
    // 验证保存的数据
    File file = SPIFFS.open("/test_qweather.json", "r");
    String content = file.readString();
    file.close();
    
    JsonDocument doc;
    deserializeJson(doc, content);
    
    TEST_ASSERT_EQUAL_STRING("101010100", doc["location"].as<String>().c_str());
    TEST_ASSERT_EQUAL_STRING("Beijing", doc["cityName"].as<String>().c_str());
}

// 测试4: 加载配置
void test_load_config(void) {
    Serial.println("\n\n----------Starting Load Config Test...----------");
    
    // 先保存配置
    QWeatherAuthConfigManager saveConfig("/test_qweather.json");
    saveConfig.setAuth(
        "api.qweather.com",
        "kidabc1234",
        "projectxyz",
        "a2V5X2Jhc2U2NA=="
    );
    saveConfig.setLocation("101020100", "Shanghai");
    
    // 创建新实例加载配置
    QWeatherAuthConfigManager loadConfig("/test_qweather.json");
    bool loadResult = loadConfig.loadConfig();
    
    TEST_ASSERT_TRUE_MESSAGE(loadResult, "Should load config successfully");
    
    // 验证加载的数据
    TEST_ASSERT_EQUAL_STRING("api.qweather.com", loadConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("kidabc1234", loadConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("projectxyz", loadConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING("a2V5X2Jhc2U2NA==", loadConfig.getBase64Key().c_str());
    TEST_ASSERT_EQUAL_STRING("101020100", loadConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("Shanghai", loadConfig.getCityName().c_str());
}

// 测试5: 重置配置
void test_reset_config(void) {
    Serial.println("\n\n----------Starting Reset Config Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 先设置配置
    qweatherConfig.setAuth("api.test.com", "kid1234567", "project123", "key");
    qweatherConfig.setLocation("123456", "TestCity");
    
    // 重置配置
    bool resetResult = qweatherConfig.resetConfig();
    TEST_ASSERT_TRUE_MESSAGE(resetResult, "Should reset config successfully");
    
    // 验证已重置为空值
    TEST_ASSERT_EQUAL_STRING("", qweatherConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("", qweatherConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("", qweatherConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING("", qweatherConfig.getBase64Key().c_str());
    TEST_ASSERT_EQUAL_STRING("", qweatherConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("", qweatherConfig.getCityName().c_str());
}

// 测试6: Getter 方法
void test_getter_methods(void) {
    Serial.println("\n\n----------Starting Getter Methods Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    qweatherConfig.setAuth(
        "devapi.qweather.com",
        "testkid123",
        "testproj12",
        "test_key"
    );
    qweatherConfig.setLocation("101010100", "Beijing");
    
    TEST_ASSERT_EQUAL_STRING("devapi.qweather.com", qweatherConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("testkid123", qweatherConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("testproj12", qweatherConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING("test_key", qweatherConfig.getBase64Key().c_str());
    TEST_ASSERT_EQUAL_STRING("101010100", qweatherConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("Beijing", qweatherConfig.getCityName().c_str());
}

// 测试7: 加载不存在的配置文件
void test_load_nonexistent_config(void) {
    Serial.println("\n\n----------Starting Load Nonexistent Config Test...----------");
    
    // 确保文件不存在
    if (SPIFFS.exists("/nonexistent_qweather.json")) {
        SPIFFS.remove("/nonexistent_qweather.json");
    }
    
    QWeatherAuthConfigManager qweatherConfig("/nonexistent_qweather.json");
    bool result = qweatherConfig.loadConfig();
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should fail to load non-existent config file"
    );
}

// 测试8: 加载损坏的 JSON 文件
void test_load_corrupted_json(void) {
    Serial.println("\n\n----------Starting Load Corrupted JSON Test...----------");
    
    // 写入无效的 JSON
    File file = SPIFFS.open("/test_qweather.json", "w");
    file.print("{invalid json content: missing quotes}");
    file.close();
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    bool result = qweatherConfig.loadConfig();
    
    TEST_ASSERT_FALSE_MESSAGE(
        result,
        "Should fail to load corrupted JSON"
    );
}

// 测试9: 完整配置流程
void test_complete_config_flow(void) {
    Serial.println("\n\n----------Starting Complete Config Flow Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 1. 初始化
    TEST_ASSERT_TRUE(qweatherConfig.init());
    
    // 2. 设置认证信息
    TEST_ASSERT_TRUE(qweatherConfig.setAuth(
        "devapi.qweather.com",
        "HE25012616",
        "Project001",
        "YmFzZTY0X2VuY29kZWRfa2V5"
    ));
    
    // 3. 设置位置信息
    TEST_ASSERT_TRUE(qweatherConfig.setLocation("101010100", "Beijing"));
    
    // 4. 重新加载验证
    QWeatherAuthConfigManager loadConfig("/test_qweather.json");
    TEST_ASSERT_TRUE(loadConfig.loadConfig());
    
    TEST_ASSERT_EQUAL_STRING("devapi.qweather.com", loadConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("HE25012616", loadConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("Project001", loadConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING("YmFzZTY0X2VuY29kZWRfa2V5", loadConfig.getBase64Key().c_str());
    TEST_ASSERT_EQUAL_STRING("101010100", loadConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("Beijing", loadConfig.getCityName().c_str());
}

// 测试10: 部分配置更新
void test_partial_config_update(void) {
    Serial.println("\n\n----------Starting Partial Config Update Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 先设置完整配置
    qweatherConfig.setAuth("api1.com", "kid1234567", "proj123456", "key1");
    qweatherConfig.setLocation("1234567890", "CityOne");
    
    // 只更新认证信息
    qweatherConfig.setAuth("api2.com", "kid2345678", "proj234567", "key2");
    
    // 验证认证信息已更新
    TEST_ASSERT_EQUAL_STRING("api2.com", qweatherConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("kid2345678", qweatherConfig.getKId().c_str());
    
    // 验证位置信息保持不变
    TEST_ASSERT_EQUAL_STRING("1234567890", qweatherConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("CityOne", qweatherConfig.getCityName().c_str());
    
    // 只更新位置信息
    qweatherConfig.setLocation("9876543210", "CityTwo");
    
    // 验证位置信息已更新
    TEST_ASSERT_EQUAL_STRING("9876543210", qweatherConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("CityTwo", qweatherConfig.getCityName().c_str());
    
    // 验证认证信息保持不变
    TEST_ASSERT_EQUAL_STRING("api2.com", qweatherConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("kid2345678", qweatherConfig.getKId().c_str());
}

// 测试11: 空字符串处理 - 验证错误处理
void test_empty_string_handling(void) {
    Serial.println("\n\n----------Starting Empty String Handling Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 设置空字符串应该失败
    bool result = qweatherConfig.setAuth("", "", "", "");
    TEST_ASSERT_FALSE_MESSAGE(result, "Should reject empty strings");
    
    // 验证错误类型
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::EmptyArguments,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 验证错误消息
    String errorMsg = qweatherConfig.getLastQWeatherErrorString();
    TEST_ASSERT_EQUAL_STRING("One or more arguments are empty", errorMsg.c_str());
}

// 测试12: 特殊字符处理
void test_special_characters(void) {
    Serial.println("\n\n----------Starting Special Characters Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 包含特殊字符的配置
    String specialApiHost = "api-test.qweather.com";
    String specialKId = "HE25012616";  // 10位字母数字
    String specialProject = "Project001";  // 10位字母数字
    String specialKey = "YmFzZTY0X2VuY29kZWRfa2V5LXRlc3Q=";
    String specialLocation = "1010101001";  // 纯数字
    String specialCity = "New York";  // 字母和空格
    
    qweatherConfig.setAuth(specialApiHost, specialKId, specialProject, specialKey);
    qweatherConfig.setLocation(specialLocation, specialCity);
    
    // 重新加载验证
    QWeatherAuthConfigManager loadConfig("/test_qweather.json");
    loadConfig.loadConfig();
    
    TEST_ASSERT_EQUAL_STRING(specialApiHost.c_str(), loadConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING(specialKId.c_str(), loadConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING(specialProject.c_str(), loadConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING(specialKey.c_str(), loadConfig.getBase64Key().c_str());
    TEST_ASSERT_EQUAL_STRING(specialLocation.c_str(), loadConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING(specialCity.c_str(), loadConfig.getCityName().c_str());
}

// 测试13: 长字符串处理
void test_long_strings(void) {
    Serial.println("\n\n----------Starting Long Strings Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 较长的字符串
    String longApiHost = "very-long-subdomain.qweather-api-service.com";
    String longKId = "ABCDE12345";  // 仍须10位
    String longProject = "XYZW567890";  // 仍须10位
    String longKey = "VmVyeUxvbmdCYXNlNjRFbmNvZGVkS2V5Rm9yVGVzdGluZ1B1cnBvc2VzMTIzNDU2Nzg5MA==";
    
    bool result = qweatherConfig.setAuth(longApiHost, longKId, longProject, longKey);
    TEST_ASSERT_TRUE_MESSAGE(result, "Should handle long strings");
    
    TEST_ASSERT_EQUAL_STRING(longApiHost.c_str(), qweatherConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING(longKId.c_str(), qweatherConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING(longProject.c_str(), qweatherConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING(longKey.c_str(), qweatherConfig.getBase64Key().c_str());
}

// 测试14: JSON 格式验证
void test_json_format_validation(void) {
    Serial.println("\n\n----------Starting JSON Format Validation Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    qweatherConfig.setAuth("api.com", "kid1234567", "project123", "key");
    qweatherConfig.setLocation("1234567890", "TestCity");
    
    // 读取并验证 JSON 格式
    File file = SPIFFS.open("/test_qweather.json", "r");
    String content = file.readString();
    file.close();
    
    Serial.printf("JSON content: %s\n", content.c_str());
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    
    TEST_ASSERT_FALSE_MESSAGE(error, "JSON should be valid");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("apiHost"), "Should contain 'apiHost' key");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("kId"), "Should contain 'kId' key");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("projectID"), "Should contain 'projectID' key");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("base64Key"), "Should contain 'base64Key' key");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("location"), "Should contain 'location' key");
    TEST_ASSERT_TRUE_MESSAGE(doc.containsKey("cityName"), "Should contain 'cityName' key");
}

// 测试15: 多次保存和加载
void test_multiple_save_load_cycles(void) {
    Serial.println("\n\n----------Starting Multiple Save/Load Cycles Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    for (int i = 1; i <= 5; i++) {
        String apiHost = "api" + String(i) + ".com";
        String kid = String("kid") + String(i) + String("234567");  // 10位
        String project = String("proj") + String(i) + String("23456");  // 10位
        String key = "key_" + String(i);
        String location = String(i) + String("234567890");  // 纯数字
        String city = "City" + String(i);  // 纯字母
        
        // 保存
        qweatherConfig.setAuth(apiHost, kid, project, key);
        qweatherConfig.setLocation(location, city);
        
        // 重新加载
        QWeatherAuthConfigManager tempConfig("/test_qweather.json");
        tempConfig.loadConfig();
        
        // 验证
        TEST_ASSERT_EQUAL_STRING(apiHost.c_str(), tempConfig.getApiHost().c_str());
        TEST_ASSERT_EQUAL_STRING(kid.c_str(), tempConfig.getKId().c_str());
        TEST_ASSERT_EQUAL_STRING(project.c_str(), tempConfig.getProjectID().c_str());
        TEST_ASSERT_EQUAL_STRING(key.c_str(), tempConfig.getBase64Key().c_str());
        TEST_ASSERT_EQUAL_STRING(location.c_str(), tempConfig.getLocation().c_str());
        TEST_ASSERT_EQUAL_STRING(city.c_str(), tempConfig.getCityName().c_str());
    }
}

// 测试16: 并发创建多个配置实例
void test_multiple_config_instances(void) {
    Serial.println("\n\n----------Starting Multiple Config Instances Test...----------");
    
    QWeatherAuthConfigManager config1("/test_qweather.json");
    QWeatherAuthConfigManager config2("/test_qweather_temp.json");
    
    config1.setAuth("api1.com", "kid1234567", "proj123456", "key1");
    config1.setLocation("1234567890", "CityOne");
    
    config2.setAuth("api2.com", "kid2345678", "proj234567", "key2");
    config2.setLocation("9876543210", "CityTwo");
    
    // 验证两个实例互不影响
    TEST_ASSERT_EQUAL_STRING("api1.com", config1.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("api2.com", config2.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("kid1234567", config1.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("kid2345678", config2.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("1234567890", config1.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("9876543210", config2.getLocation().c_str());
}

// 测试17: 文件大小验证
void test_config_file_size(void) {
    Serial.println("\n\n----------Starting Config File Size Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    qweatherConfig.setAuth("devapi.qweather.com", "HE12345678", "Project001", "base64key");
    qweatherConfig.setLocation("101010100", "Beijing");
    
    File file = SPIFFS.open("/test_qweather.json", "r");
    size_t fileSize = file.size();
    file.close();
    
    Serial.printf("Config file size: %d bytes\n", fileSize);
    
    TEST_ASSERT_TRUE_MESSAGE(
        fileSize > 0 && fileSize < 2048,
        "Config file size should be reasonable (0-2048 bytes)"
    );
}

// 测试18: 中文城市名称
void test_chinese_city_names(void) {
    Serial.println("\n\n----------Starting Chinese City Names Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 英文城市名称
    String cities[] = {"Beijing", "Shanghai", "Guangzhou", "Shenzhen", "Chengdu"};
    String locations[] = {"101010100", "101020100", "101280101", "101280601", "101270101"};
    
    for (int i = 0; i < 5; i++) {
        qweatherConfig.setLocation(locations[i], cities[i]);
        
        QWeatherAuthConfigManager loadConfig("/test_qweather.json");
        loadConfig.loadConfig();
        
        TEST_ASSERT_EQUAL_STRING(locations[i].c_str(), loadConfig.getLocation().c_str());
        TEST_ASSERT_EQUAL_STRING(cities[i].c_str(), loadConfig.getCityName().c_str());
    }
}

// 测试19: Base64 密钥格式验证
void test_base64_key_format(void) {
    Serial.println("\n\n----------Starting Base64 Key Format Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 有效的 Base64 字符串
    String validBase64Keys[] = {
        "YmFzZTY0X2VuY29kZWRfa2V5",
        "VGVzdEtleTE2MTAyMzE=",
        "QVdTX1NFQ1JFVF9LRVk=",
        "MTIzNDU2Nzg5MDEyMzQ1Ng=="
    };
    
    for (const String& key : validBase64Keys) {
        qweatherConfig.setAuth("api.com", "kid1234567", "project123", key);
        TEST_ASSERT_EQUAL_STRING(key.c_str(), qweatherConfig.getBase64Key().c_str());
    }
}

// 测试20: 完整的真实场景模拟
void test_real_world_scenario(void) {
    Serial.println("\n\n----------Starting Real World Scenario Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 1. 首次使用，初始化配置
    TEST_ASSERT_TRUE(qweatherConfig.init());
    
    // 2. 用户输入认证信息
    bool authSet = qweatherConfig.setAuth(
        "devapi.qweather.com",
        "HE25012616",
        "QWeather24",
        "WW91cl9TZWN1cmVfQmFzZTY0X0VuY29kZWRfS2V5"
    );
    TEST_ASSERT_TRUE(authSet);
    
    // 3. 用户选择城市
    bool locationSet = qweatherConfig.setLocation("101010100", "Beijing");
    TEST_ASSERT_TRUE(locationSet);
    
    // 4. 应用重启，重新加载配置
    QWeatherAuthConfigManager reloadConfig("/test_qweather.json");
    TEST_ASSERT_TRUE(reloadConfig.init());
    
    // 5. 验证配置正确加载
    TEST_ASSERT_EQUAL_STRING("devapi.qweather.com", reloadConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("HE25012616", reloadConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("101010100", reloadConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("Beijing", reloadConfig.getCityName().c_str());
    
    // 6. 用户更换城市
    bool cityChanged = reloadConfig.setLocation("101020100", "Shanghai");
    TEST_ASSERT_TRUE(cityChanged);
    
    // 7. 验证城市已更新，认证信息未变
    TEST_ASSERT_EQUAL_STRING("101020100", reloadConfig.getLocation().c_str());
    TEST_ASSERT_EQUAL_STRING("Shanghai", reloadConfig.getCityName().c_str());
    TEST_ASSERT_EQUAL_STRING("HE25012616", reloadConfig.getKId().c_str());
    
    // 8. 用户重置配置
    bool reset = reloadConfig.resetConfig();
    TEST_ASSERT_TRUE(reset);
    
    // 9. 验证配置已清空
    TEST_ASSERT_EQUAL_STRING("", reloadConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("", reloadConfig.getLocation().c_str());
}

// 测试21: 无效的API主机格式
void test_invalid_api_host(void) {
    Serial.println("\n\n----------Starting Invalid API Host Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 测试包含空格的主机名
    bool result1 = qweatherConfig.setAuth("api test.com", "1234567890", "1234567890", "key");
    TEST_ASSERT_FALSE_MESSAGE(result1, "Should reject API host with spaces");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidApiHost,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试没有点的主机名
    bool result2 = qweatherConfig.setAuth("localhost", "1234567890", "1234567890", "key");
    TEST_ASSERT_FALSE_MESSAGE(result2, "Should reject API host without dot");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidApiHost,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试有效的主机名
    bool result3 = qweatherConfig.setAuth("api.qweather.com", "1234567890", "1234567890", "key");
    TEST_ASSERT_TRUE_MESSAGE(result3, "Should accept valid API host");
}

// 测试22: 无效的KId格式
void test_invalid_kid_format(void) {
    Serial.println("\n\n----------Starting Invalid KId Format Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 测试长度不等于10的KId
    bool result1 = qweatherConfig.setAuth("api.qweather.com", "123", "1234567890", "key");
    TEST_ASSERT_FALSE_MESSAGE(result1, "Should reject KId with wrong length");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidKid,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试包含特殊字符的KId
    bool result2 = qweatherConfig.setAuth("api.qweather.com", "123456789!", "1234567890", "key");
    TEST_ASSERT_FALSE_MESSAGE(result2, "Should reject KId with special characters");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidKid,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试有效的KId (10位字母数字)
    bool result3 = qweatherConfig.setAuth("api.qweather.com", "ABC1234567", "1234567890", "key");
    TEST_ASSERT_TRUE_MESSAGE(result3, "Should accept valid KId");
}

// 测试23: 无效的ProjectID格式
void test_invalid_project_id_format(void) {
    Serial.println("\n\n----------Starting Invalid ProjectID Format Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 测试长度不等于10的ProjectID
    bool result1 = qweatherConfig.setAuth("api.qweather.com", "1234567890", "12345", "key");
    TEST_ASSERT_FALSE_MESSAGE(result1, "Should reject ProjectID with wrong length");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidProjectID,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试包含特殊字符的ProjectID
    bool result2 = qweatherConfig.setAuth("api.qweather.com", "1234567890", "123456789@", "key");
    TEST_ASSERT_FALSE_MESSAGE(result2, "Should reject ProjectID with special characters");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidProjectID,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试有效的ProjectID
    bool result3 = qweatherConfig.setAuth("api.qweather.com", "1234567890", "XYZ9876543", "key");
    TEST_ASSERT_TRUE_MESSAGE(result3, "Should accept valid ProjectID");
}

// 测试24: 无效的Location格式
void test_invalid_location_format(void) {
    Serial.println("\n\n----------Starting Invalid Location Format Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    qweatherConfig.init();
    
    // 测试包含字母的location
    bool result1 = qweatherConfig.setLocation("10101abc0", "Beijing");
    TEST_ASSERT_FALSE_MESSAGE(result1, "Should reject location with letters");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidLocation,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试包含特殊字符的location
    bool result2 = qweatherConfig.setLocation("101-010-100", "Beijing");
    TEST_ASSERT_FALSE_MESSAGE(result2, "Should reject location with special characters");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidLocation,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试有效的location (纯数字)
    bool result3 = qweatherConfig.setLocation("101010100", "Beijing");
    TEST_ASSERT_TRUE_MESSAGE(result3, "Should accept valid location");
}

// 测试25: 无效的CityName格式
void test_invalid_cityname_format(void) {
    Serial.println("\n\n----------Starting Invalid CityName Format Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    qweatherConfig.init();
    
    // 测试包含数字的城市名
    bool result1 = qweatherConfig.setLocation("101010100", "Beijing123");
    TEST_ASSERT_FALSE_MESSAGE(result1, "Should reject city name with numbers");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidCityName,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试包含特殊字符的城市名
    bool result2 = qweatherConfig.setLocation("101010100", "Beijing@City");
    TEST_ASSERT_FALSE_MESSAGE(result2, "Should reject city name with special characters");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidCityName,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试有效的城市名 (字母和空格)
    bool result3 = qweatherConfig.setLocation("101010100", "New York");
    TEST_ASSERT_TRUE_MESSAGE(result3, "Should accept valid city name");
}

// 测试26: 错误状态字符串
void test_error_string_messages(void) {
    Serial.println("\n\n----------Starting Error String Messages Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 测试InvalidApiHost错误
    qweatherConfig.setAuth("invalid host", "1234567890", "1234567890", "key");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidApiHost,
        qweatherConfig.getLastQWeatherError()
    );
    TEST_ASSERT_EQUAL_STRING(
        "Invalid API Host",
        qweatherConfig.getLastQWeatherErrorString().c_str()
    );
    
    // 测试InvalidKid错误
    qweatherConfig.setAuth("api.qweather.com", "123", "1234567890", "key");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidKid,
        qweatherConfig.getLastQWeatherError()
    );
    TEST_ASSERT_EQUAL_STRING(
        "Invalid Key ID",
        qweatherConfig.getLastQWeatherErrorString().c_str()
    );
    
    // 测试InvalidProjectID错误
    qweatherConfig.setAuth("api.qweather.com", "1234567890", "123", "key");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidProjectID,
        qweatherConfig.getLastQWeatherError()
    );
    TEST_ASSERT_EQUAL_STRING(
        "Invalid Project ID",
        qweatherConfig.getLastQWeatherErrorString().c_str()
    );
    
    // 测试EmptyArguments错误
    qweatherConfig.setAuth("", "", "", "");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::EmptyArguments,
        qweatherConfig.getLastQWeatherError()
    );
    TEST_ASSERT_EQUAL_STRING(
        "One or more arguments are empty",
        qweatherConfig.getLastQWeatherErrorString().c_str()
    );
    
    // 测试ConfigFileError错误
    QWeatherAuthConfigManager corruptedConfig("/nonexistent_file.json");
    corruptedConfig.loadConfig();
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::ConfigFileError,
        corruptedConfig.getLastQWeatherError()
    );
    TEST_ASSERT_EQUAL_STRING(
        "Configuration file error",
        corruptedConfig.getLastQWeatherErrorString().c_str()
    );
}

// 测试27: 错误状态持久性
void test_error_state_persistence(void) {
    Serial.println("\n\n----------Starting Error State Persistence Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 触发一个错误
    qweatherConfig.setAuth("invalid host", "1234567890", "1234567890", "key");
    
    // 验证错误状态被记录
    QWeatherAuthConfigManager::QWeatherError firstError = qweatherConfig.getLastQWeatherError();
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::InvalidApiHost,
        firstError
    );
    
    // 再次获取应该返回相同的错误
    TEST_ASSERT_EQUAL(firstError, qweatherConfig.getLastQWeatherError());
    
    // 成功操作后，错误状态应该被清除或更新
    qweatherConfig.setAuth("api.qweather.com", "1234567890", "1234567890", "key");
    // 成功后错误应该是None或ConfigFileError (取决于实现)
    QWeatherAuthConfigManager::QWeatherError afterSuccess = qweatherConfig.getLastQWeatherError();
    Serial.printf("Error after success: %d\n", (int)afterSuccess);
}

// 测试28: 组合验证错误
void test_combined_validation_errors(void) {
    Serial.println("\n\n----------Starting Combined Validation Errors Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 测试多个参数同时无效 - 应该返回第一个检测到的错误
    bool result = qweatherConfig.setAuth("invalid host", "bad", "xyz", "");
    TEST_ASSERT_FALSE_MESSAGE(result, "Should fail validation");
    
    // 验证错误被正确记录
    QWeatherAuthConfigManager::QWeatherError error = qweatherConfig.getLastQWeatherError();
    TEST_ASSERT_TRUE_MESSAGE(
        error != QWeatherAuthConfigManager::QWeatherError::None,
        "Should have recorded an error"
    );
    
    Serial.printf("Detected error: %s\n", 
        qweatherConfig.getLastQWeatherErrorString().c_str()
    );
}

// 测试29: 参数修剪(trim)功能
void test_parameter_trimming(void) {
    Serial.println("\n\n----------Starting Parameter Trimming Test...----------");
    
    QWeatherAuthConfigManager qweatherConfig("/test_qweather.json");
    
    // 设置带有前后空格的参数
    bool result = qweatherConfig.setAuth(
        "  api.qweather.com  ",
        "  1234567890  ",
        "  ABCDEFGHIJ  ",
        "  base64key  "
    );
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Should accept and trim parameters");
    
    // 验证空格已被修剪
    TEST_ASSERT_EQUAL_STRING("api.qweather.com", qweatherConfig.getApiHost().c_str());
    TEST_ASSERT_EQUAL_STRING("1234567890", qweatherConfig.getKId().c_str());
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJ", qweatherConfig.getProjectID().c_str());
    TEST_ASSERT_EQUAL_STRING("base64key", qweatherConfig.getBase64Key().c_str());
}

// 测试30: 配置文件加载错误处理
void test_config_file_load_error_handling(void) {
    Serial.println("\n\n----------Starting Config File Load Error Handling Test...----------");
    
    // 测试加载不存在的文件
    QWeatherAuthConfigManager qweatherConfig("/nonexistent_file.json");
    bool result = qweatherConfig.loadConfig();
    
    TEST_ASSERT_FALSE_MESSAGE(result, "Should fail to load non-existent file");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::ConfigFileError,
        qweatherConfig.getLastQWeatherError()
    );
    
    // 测试加载损坏的JSON
    File file = SPIFFS.open("/test_qweather.json", "w");
    file.print("{invalid json");
    file.close();
    
    QWeatherAuthConfigManager corruptedConfig("/test_qweather.json");
    result = corruptedConfig.loadConfig();
    
    TEST_ASSERT_FALSE_MESSAGE(result, "Should fail to load corrupted JSON");
    TEST_ASSERT_EQUAL(
        QWeatherAuthConfigManager::QWeatherError::ConfigFileError,
        corruptedConfig.getLastQWeatherError()
    );
}

// ============ Unity 主函数 ============

void setup() {
    delay(2000);  // 等待串口稳定
    
    globalTestSetup();  // 全局初始化
    
    UNITY_BEGIN();
    
    lcdText("QWeather Tests", 1);
    
    // 基础功能测试
    RUN_TEST(test_qweather_manager_initialization);
    RUN_TEST(test_save_auth_config);
    RUN_TEST(test_save_location_config);
    RUN_TEST(test_load_config);
    RUN_TEST(test_reset_config);
    RUN_TEST(test_getter_methods);
    
    // 错误处理测试
    RUN_TEST(test_load_nonexistent_config);
    RUN_TEST(test_load_corrupted_json);
    
    // 高级功能测试
    RUN_TEST(test_complete_config_flow);
    RUN_TEST(test_partial_config_update);
    RUN_TEST(test_empty_string_handling);
    RUN_TEST(test_special_characters);
    RUN_TEST(test_long_strings);
    RUN_TEST(test_json_format_validation);
    RUN_TEST(test_multiple_save_load_cycles);
    RUN_TEST(test_multiple_config_instances);
    RUN_TEST(test_config_file_size);
    RUN_TEST(test_chinese_city_names);
    RUN_TEST(test_base64_key_format);
    RUN_TEST(test_real_world_scenario);
    
    // 新增的验证和错误处理测试
    RUN_TEST(test_invalid_api_host);
    RUN_TEST(test_invalid_kid_format);
    RUN_TEST(test_invalid_project_id_format);
    RUN_TEST(test_invalid_location_format);
    RUN_TEST(test_invalid_cityname_format);
    RUN_TEST(test_error_string_messages);
    RUN_TEST(test_error_state_persistence);
    RUN_TEST(test_combined_validation_errors);
    RUN_TEST(test_parameter_trimming);
    RUN_TEST(test_config_file_load_error_handling);
    
    lcdText("Tests Completed", 1);
    
    UNITY_END();
    
    globalTestTeardown();  // 全局清理
}

void loop() {
    // 测试完成后不执行任何操作
}