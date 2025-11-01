#include "wifi_config.h"

// 配网模式使用的 Web 服务器（监听端口 80）
WebServer AP_server(80);

// 用户通过网页输入的 SSID 与密码
String ssid_input, password_input;

// 保存的 WiFi SSID 和密码（初始化为空字符串）
String savedSSID = "", savedPassword = "";

// 当前是否处于配网模式的标志位
bool inConfigMode = false;

// 保存WiFi信息
void saveWiFiCredentials(const String& ssid, const String& password) {
  File file = SPIFFS.open("/wifi.txt", "w");
  if (!file) {
      Serial.println("保存WiFi信息失败, 无法打开文件");
      lcd_text("Save WiFi Fail", 1);
      lcd_text("Check FS/Retry", 2);
      return;
  }
  if (!file.println(ssid)) {
      Serial.println("保存WiFi信息失败, 写入SSID失败");
      lcd_text("Save WiFi Fail", 1);
      lcd_text("Write Err", 2);
      file.close();
      return;
  }
  if (!file.println(password)) {
      Serial.println("保存WiFi信息失败, 写入密码失败");
      lcd_text("Save WiFi Fail", 1);
      lcd_text("Write Err", 2);
      file.close();
      return;
  }
  file.flush();
  file.close();

  Serial.println("WiFi 信息已保存");
  lcd_text("Config Saved", 1);
  lcd_text("Restarting...", 2);
}

// 加载WiFi信息
void loadWiFiCredentials() {
    if (!SPIFFS.exists("/wifi.txt")) {
        savedSSID = "";
        savedPassword = "";
        return;
    }
    File file = SPIFFS.open("/wifi.txt", "r");
    if (file) {
        savedSSID = file.readStringUntil('\n');
        Serial.println("Loaded SSID: " + savedSSID);
        savedSSID.trim();
        savedPassword = file.readStringUntil('\n');
        savedPassword.trim();
        file.close();
    } else {
        savedSSID = "";
        savedPassword = "";
    }
}


// 配网网页
void handleRoot() {
  AP_server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html lang="zh-CN">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>WiFi 配置</title>
      <style>
        body {
          font-family: "Segoe UI", sans-serif;
          background-color: #f2f2f2;
          display: flex;
          justify-content: center;
          align-items: center;
          height: 100vh;
          margin: 0;
        }
        .container {
          background-color: #fff;
          padding: 2em;
          border-radius: 12px;
          box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
          max-width: 90%;
          width: 400px;
        }
        h2 {
          text-align: center;
          color: #333;
        }
        label {
          display: block;
          margin: 1em 0 0.3em;
          color: #555;
        }
        input[type="text"], input[type="password"] {
          width: 100%;
          padding: 0.6em;
          font-size: 1em;
          border: 1px solid #ccc;
          border-radius: 8px;
          box-sizing: border-box;
        }
        input[type="submit"] {
          margin-top: 1.5em;
          width: 100%;
          padding: 0.7em;
          font-size: 1em;
          background-color: #4CAF50;
          color: white;
          border: none;
          border-radius: 8px;
          cursor: pointer;
        }
        input[type="submit"]:hover {
          background-color: #45a049;
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h2>连接到 WiFi</h2>
        <form action="/set">
          <label for="ssid">WiFi 名称 (SSID):</label>
          <input type="text" id="ssid" name="ssid" required>

          <label for="password">密码:</label>
          <input type="password" id="password" name="password" required>

          <input type="submit" value="保存并重启">
        </form>
      </div>
    </body>
    </html>
  )rawliteral");
}



// 保存按钮触发器
void handleSet() {
    ssid_input = AP_server.arg("ssid");
    password_input = AP_server.arg("password");
    saveWiFiCredentials(ssid_input, password_input);
    AP_server.send(200, "text/plain; charset=UTF-8", "保存成功，正在重启...");
    delay(2000);
    ESP.restart();
}

// 进入配网
void enterConfigMode() {
  inConfigMode = true;
  Serial.println("进入配网模式");
  WiFi.softAP("1602A_Config");
  Serial.println("配网网页开启于IP: " + WiFi.softAPIP().toString());

  // 扫描WiFi并展示列表
  AP_server.on("/", [](){
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>WiFi 配置</title>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
  html += "<style>body{font-family:Segoe UI,sans-serif;background:#e9f0fa;margin:0;}";
  html += ".container{background:#fff;padding:1.2em 1.2em;border-radius:14px;box-shadow:0 4px 16px rgba(0,0,0,0.10);max-width:92vw;width:320px;margin:1.2em auto;}h2{text-align:center;color:#1976D2;margin-bottom:0.8em;font-size:1.2em;}label{display:block;margin:0.8em 0 0.2em;color:#1976D2;font-weight:bold;font-size:0.98em;}input[type=password],input[type=text],select{width:100%;padding:0.5em;font-size:0.98em;border:1px solid #bcdffb;border-radius:8px;box-sizing:border-box;margin-bottom:0.8em;}input[type=submit]{margin-top:0.8em;width:100%;padding:0.6em;font-size:1em;background:linear-gradient(90deg,#2196F3,#4CAF50);color:white;border:none;border-radius:8px;cursor:pointer;transition:background 0.2s;}input[type=submit]:hover{background:linear-gradient(90deg,#1976D2,#388E3C);}ul{padding:0;}li{margin:0.4em 0;}select{background:#f6fbff;}form{margin-bottom:0;}@media(max-width:600px){.container{padding:0.7em;width:98vw;min-width:0;}}";
  html += "</style></head><body><div class='container'><h2>WiFi智能配网</h2>";
  html += "<form action='/wifi_scan' method='POST'><input type='submit' value='扫描附近WiFi'></form>";
  html += "<form action='/wifi_set' method='POST' style='margin-top:1.5em;'><label for='manualssid'>手动输入WiFi名称:</label><input type='text' name='ssid' id='manualssid' placeholder='请输入SSID'><label for='password'>密码:</label><input type='password' name='password' id='password' required><input type='submit' value='连接WiFi'></form>";
  html += "<p style='color:#888;font-size:0.95em;text-align:center;margin-top:1em;'>可扫描或手动输入WiFi名称，适配手机/平板/PC</p>";
  html += "</div></body></html>";
    AP_server.send(200, "text/html; charset=utf-8", html);
  });

  AP_server.on("/wifi_scan", [](){
    int n = WiFi.scanNetworks();
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>WiFi选择</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1.0'>";
    html += "<style>body{font-family:Segoe UI,sans-serif;background:#e9f0fa;margin:0;}";
  html += ".container{background:#fff;padding:1.2em 1.2em;border-radius:14px;box-shadow:0 4px 16px rgba(0,0,0,0.10);max-width:92vw;width:320px;margin:1.2em auto;}h2{text-align:center;color:#1976D2;margin-bottom:0.8em;font-size:1.2em;}label{display:block;margin:0.8em 0 0.2em;color:#1976D2;font-weight:bold;font-size:0.98em;}input[type=password],input[type=text],select{width:100%;padding:0.5em;font-size:0.98em;border:1px solid #bcdffb;border-radius:8px;box-sizing:border-box;margin-bottom:0.8em;}input[type=submit]{margin-top:0.8em;width:100%;padding:0.6em;font-size:1em;background:linear-gradient(90deg,#2196F3,#4CAF50);color:white;border:none;border-radius:8px;cursor:pointer;transition:background 0.2s;}input[type=submit]:hover{background:linear-gradient(90deg,#1976D2,#388E3C);}ul{padding:0;}li{margin:0.4em 0;}select{background:#f6fbff;}form{margin-bottom:0;}@media(max-width:600px){.container{padding:0.7em;width:98vw;min-width:0;}}";
    html += "</style></head><body><div class='container'><h2>WiFi智能配网</h2>";
    if (n == 0) {
      html += "<p style='color:#d32f2f;text-align:center;'>未发现WiFi，请重试。</p>";
    } else {
      html += "<form action='/wifi_set' method='POST'><label for='ssid'>WiFi列表:</label><select name='ssid' id='ssid'>";
      for (int i = 0; i < n; ++i) {
        html += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
      }
      html += "</select><label for='password'>密码:</label><input type='password' name='password' id='password' required><input type='submit' value='连接WiFi'></form>";
    }
    html += "<form action='/wifi_set' method='POST' style='margin-top:1.5em;'><label for='manualssid'>手动输入WiFi名称:</label><input type='text' name='ssid' id='manualssid' placeholder='请输入SSID'><label for='password'>密码:</label><input type='password' name='password' id='password' required><input type='submit' value='连接WiFi'></form>";
    html += "<p style='color:#888;font-size:0.95em;text-align:center;margin-top:1em;'>可扫描或手动输入WiFi名称，适配手机/平板/PC</p>";
    html += "</div></body></html>";
    AP_server.send(200, "text/html; charset=utf-8", html);
  });

  AP_server.on("/wifi_set", [](){
    String ssid = AP_server.arg("ssid");
    String password = AP_server.arg("password");
    saveWiFiCredentials(ssid, password);
  AP_server.send(200, "text/html; charset=utf-8", "<div style='font-family:Segoe UI,sans-serif;background:#e9f0fa;height:100vh;display:flex;justify-content:center;align-items:center;'><div style='background:#fff;padding:2em 2.5em;border-radius:16px;box-shadow:0 8px 32px rgba(0,0,0,0.12);max-width:95%;width:420px;text-align:center;'><h2 style='color:#1976D2;'>WiFi信息已保存</h2><p style='color:#388E3C;'>设备正在重启，请稍候...</p></div></div>");
    delay(2000);
    ESP.restart();
  });

  AP_server.begin();

  // 在屏幕上显示ip
  lcd_text("Connect to AP",1);
  lcd_text("IP:" + WiFi.softAPIP().toString(),2);
}

void connectToWiFi() {
    loadWiFiCredentials();

    if (savedSSID == "") {
        Serial.println("无保存信息，进入配网");
        updateColor(CRGB::Purple);  // 初次配网紫灯
        enterConfigMode();
        return;
    }

    updateColor(CRGB::Blue);  // 连接中蓝灯
    Serial.print("正在连接: ");
    Serial.println(savedSSID);

    // 在屏幕上显示状态
    lcd_text("WIFI connecting",1);
    lcd_text(" ",2);

    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

    unsigned long startTime = millis();
    int fadeStep = 2;
    uint8_t brightness = 64;
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        brightness += fadeStep;

        if (brightness == 0 || brightness == 192) {
            fadeStep = -fadeStep;
        }

        if((millis() - startTime) % 1000 == 0){
            Serial.print(".");
        }
        updateBrightness(brightness);
        delay(5);
    }
    updateBrightness(128);

    if (WiFi.status() == WL_CONNECTED) {
        updateColor(CRGB::Green);  // 连接成功绿灯
        Serial.println("\n已连接");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        // 在屏幕上显示ip
        lcd_text("SSID:" + savedSSID ,1);
        lcd_text("IP:" + WiFi.localIP().toString(),2);
    } else {
        Serial.println("\n连接失败, 进入配网");
        updateColor(CRGB::Red);  // 失败变红
        enterConfigMode();
    }
}

// 初始化wifi
void wifiinit(){
  // 连接WiFi
  if (digitalRead(BUTTEN_CENTER)) {
      Serial.println("按键按下，强制进入配网模式");
      updateColor(CRGB::Purple);  // 手动配网紫灯
      enterConfigMode();
  } else {
      connectToWiFi();
  }
    
  server.begin();
}