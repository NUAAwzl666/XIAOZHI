#ifndef DEEPSEEK_CLIENT_H
#define DEEPSEEK_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class DeepSeekClient {
private:
    WiFiClientSecure client;
    HTTPClient http;
    String apiKey;
    String baseUrl;
    
public:
    DeepSeekClient();
    ~DeepSeekClient();
    
    // 初始化方法
    bool initialize(const String& apiKey, const String& baseUrl = "https://api.deepseek.com");
    
    // 对话方法
    String chat(const String& message, const String& systemPrompt = "");
    String chatStreaming(const String& message, const String& systemPrompt = "");
    
    // 异步对话方法
    bool startChat(const String& message, const String& systemPrompt = "");
    String getResponse();
    bool isResponseReady();
    
    // 状态检查
    bool isConnected();
    
private:
    String buildChatRequest(const String& message, const String& systemPrompt);
    String parseResponse(const String& response);
    bool sendRequest(const String& payload);
};

extern DeepSeekClient deepSeekClient;

#endif