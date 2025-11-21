#ifndef DEEPSEEK_CLIENT_H
#define DEEPSEEK_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class DeepSeekClient {
private:
    String apiKey;
    String baseUrl;
    WiFiClientSecure client;
    
    // 对话历史管理
    static const int MAX_HISTORY = 10;
    String conversationHistory[MAX_HISTORY * 2]; // 用户和助手的对话对
    int historyCount;
    
    // 私有方法
    String buildRequestBody(const String& userMessage);
    String parseResponse(const String& response);
    void addToHistory(const String& userMessage, const String& assistantResponse);
    void cleanOldHistory();

public:
    DeepSeekClient();
    ~DeepSeekClient();
    
    // 初始化
    bool begin(const String& key);
    
    // 基础对话
    String chat(const String& message);
    String chatStream(const String& message); // 流式对话（未来实现）
    
    // 会话管理
    void clearHistory();
    int getHistoryCount() const;
    String getHistoryItem(int index) const;
    
    // 配置
    void setModel(const String& model);
    void setTemperature(float temp);
    void setMaxTokens(int tokens);
    
    // 状态检查
    bool isConnected() const;
    String getLastError() const;
    
private:
    String currentModel;
    float temperature;
    int maxTokens;
    String lastError;
    bool initialized;
};

#endif