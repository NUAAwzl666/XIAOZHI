// 简化的百度语音测试程序
// 用于诊断百度语音服务初始化问题

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi配置
const char* ssid = "1234";
const char* password = "123456sss";

// 百度API配置
const char* baidu_app_id = "120197558";
const char* baidu_api_key = "iw0uVTjdNE3EHj5I0ZliSj8Z";
const char* baidu_secret_key = "M34aLHQUH2mkK3MDLQ540jfpiVUJpz6n";

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n========================================");
    Serial.println("=== 百度语音服务测试程序 ===");
    Serial.println("========================================");
    
    // 连接WiFi
    Serial.println("[WIFI] 连接WiFi...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] ✓ WiFi连接成功!");
        Serial.printf("[WIFI] IP地址: %s\n", WiFi.localIP().toString().c_str());
        
        // 测试百度API
        testBaiduAPI();
    } else {
        Serial.println("\n[WIFI] ✗ WiFi连接失败!");
    }
}

void testBaiduAPI() {
    Serial.println("\n========================================");
    Serial.println("测试百度语音API访问令牌获取");
    Serial.println("========================================");
    
    Serial.printf("APP_ID: %s\n", baidu_app_id);
    Serial.printf("API_KEY: %s\n", String(baidu_api_key).substring(0, 8).c_str());
    Serial.printf("SECRET_KEY: %s\n", String(baidu_secret_key).substring(0, 8).c_str());
    
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);
    
    HTTPClient http;
    String token_url = "https://aip.baidubce.com/oauth/2.0/token";
    
    Serial.printf("连接到: %s\n", token_url.c_str());
    
    if (!http.begin(client, token_url)) {
        Serial.println("✗ HTTP客户端初始化失败");
        return;
    }
    
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.setTimeout(15000);
    
    String postData = "grant_type=client_credentials";
    postData += "&client_id=" + String(baidu_api_key);
    postData += "&client_secret=" + String(baidu_secret_key);
    
    Serial.printf("发送POST数据 (长度: %d bytes):\n", postData.length());
    Serial.println(postData);
    
    Serial.printf("可用内存: %d bytes\n", ESP.getFreeHeap());
    
    Serial.println("发送HTTP请求...");
    int httpCode = http.POST(postData);
    
    Serial.printf("HTTP响应码: %d\n", httpCode);
    
    if (httpCode > 0) {
        String response = http.getString();
        Serial.printf("响应长度: %d bytes\n", response.length());
        
        if (response.length() > 0) {
            Serial.println("原始响应:");
            Serial.println(response);
            
            if (httpCode == 200) {
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, response);
                
                if (error) {
                    Serial.printf("JSON解析错误: %s\n", error.c_str());
                } else {
                    if (doc.containsKey("access_token")) {
                        String token = doc["access_token"];
                        int expires_in = doc["expires_in"];
                        Serial.printf("✓ 成功获取访问令牌: %s...\n", token.substring(0, 20).c_str());
                        Serial.printf("✓ 有效期: %d 秒\n", expires_in);
                    } else {
                        Serial.println("✗ 响应中缺少access_token");
                        if (doc.containsKey("error_description")) {
                            Serial.printf("错误描述: %s\n", doc["error_description"].as<String>().c_str());
                        }
                    }
                }
            } else {
                Serial.printf("✗ HTTP错误: %d\n", httpCode);
            }
        } else {
            Serial.println("✗ 响应为空");
        }
    } else {
        Serial.printf("✗ HTTP请求失败: %d\n", httpCode);
    }
    
    http.end();
    Serial.printf("测试完成，可用内存: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    // 保持空循环
    delay(1000);
}