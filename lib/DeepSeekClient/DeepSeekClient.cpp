#include "DeepSeekClient.h"#include "DeepSeekClient.h"

#include "config.h"

DeepSeekClient::DeepSeekClient() {

    baseUrl = "https://api.deepseek.com/v1/chat/completions";DeepSeekClient deepSeekClient;

    currentModel = "deepseek-chat";

    temperature = 0.7;DeepSeekClient::DeepSeekClient() {

    maxTokens = 1000;    client.setInsecure(); // 在生产环境中应该使用证书验证

    historyCount = 0;}

    initialized = false;

    lastError = "";DeepSeekClient::~DeepSeekClient() {

}    http.end();

}

DeepSeekClient::~DeepSeekClient() {

    client.stop();bool DeepSeekClient::initialize(const String& apiKey, const String& baseUrl) {

}    this->apiKey = apiKey;

    this->baseUrl = baseUrl;

bool DeepSeekClient::begin(const String& key) {    

    if (key.length() == 0) {    Serial.println("初始化DeepSeek客户端...");

        lastError = "API key不能为空";    

        return false;    if (apiKey.length() == 0) {

    }        Serial.println("错误：DeepSeek API密钥为空");

            return false;

    apiKey = key;    }

    client.setInsecure(); // 用于HTTPS连接    

    initialized = true;    Serial.println("DeepSeek客户端初始化成功");

        return true;

    Serial.println("[DeepSeek] 客户端初始化成功");}

    return true;

}String DeepSeekClient::chat(const String& message, const String& systemPrompt) {

    if (!isConnected()) {

String DeepSeekClient::buildRequestBody(const String& userMessage) {        Serial.println("DeepSeek客户端未连接");

    DynamicJsonDocument doc(2048);        return "抱歉，我现在无法连接到AI服务。";

        }

    doc["model"] = currentModel;    

    doc["temperature"] = temperature;    Serial.println("发送消息到DeepSeek: " + message);

    doc["max_tokens"] = maxTokens;    

        String payload = buildChatRequest(message, systemPrompt);

    JsonArray messages = doc.createNestedArray("messages");    

        http.begin(client, baseUrl + "/v1/chat/completions");

    // 添加系统提示    http.addHeader("Content-Type", "application/json");

    JsonObject systemMsg = messages.createNestedObject();    http.addHeader("Authorization", "Bearer " + apiKey);

    systemMsg["role"] = "system";    

    systemMsg["content"] = "你是小智，一个友好、有趣且富有同情心的AI语音助手。请用简洁、自然的中文回答，就像朋友之间的对话一样。回答控制在50字以内，除非需要详细解释。";    int httpResponseCode = http.POST(payload);

        String response = "";

    // 添加历史对话    

    for (int i = 0; i < historyCount; i += 2) {    if (httpResponseCode == 200) {

        if (i + 1 < historyCount) {        response = http.getString();

            JsonObject userMsg = messages.createNestedObject();        response = parseResponse(response);

            userMsg["role"] = "user";        Serial.println("DeepSeek响应: " + response);

            userMsg["content"] = conversationHistory[i];    } else {

                    Serial.printf("DeepSeek请求失败，错误码: %d\n", httpResponseCode);

            JsonObject assistantMsg = messages.createNestedObject();        response = "抱歉，AI服务暂时不可用。";

            assistantMsg["role"] = "assistant";    }

            assistantMsg["content"] = conversationHistory[i + 1];    

        }    http.end();

    }    return response;

    }

    // 添加当前用户消息

    JsonObject currentMsg = messages.createNestedObject();String DeepSeekClient::chatStreaming(const String& message, const String& systemPrompt) {

    currentMsg["role"] = "user";    // 流式响应实现（简化版本）

    currentMsg["content"] = userMessage;    return chat(message, systemPrompt);

    }

    String output;

    serializeJson(doc, output);bool DeepSeekClient::startChat(const String& message, const String& systemPrompt) {

    return output;    // 异步聊天实现

}    return true;

}

String DeepSeekClient::parseResponse(const String& response) {

    DynamicJsonDocument doc(2048);String DeepSeekClient::getResponse() {

    DeserializationError error = deserializeJson(doc, response);    // 获取异步响应

        return "";

    if (error) {}

        lastError = "JSON解析错误: " + String(error.c_str());

        return "抱歉，我现在无法理解你的问题。";bool DeepSeekClient::isResponseReady() {

    }    // 检查异步响应是否准备就绪

        return false;

    if (doc.containsKey("error")) {}

        String errorMsg = doc["error"]["message"];

        lastError = "API错误: " + errorMsg;bool DeepSeekClient::isConnected() {

        return "抱歉，服务暂时不可用。";    return WiFi.status() == WL_CONNECTED && apiKey.length() > 0;

    }}

    

    if (doc.containsKey("choices") && doc["choices"].size() > 0) {String DeepSeekClient::buildChatRequest(const String& message, const String& systemPrompt) {

        String content = doc["choices"][0]["message"]["content"];    DynamicJsonDocument doc(2048);

        return content;    

    }    doc["model"] = "deepseek-chat";

        doc["max_tokens"] = 1000;

    lastError = "响应格式错误";    doc["temperature"] = 0.7;

    return "抱歉，我没有收到有效的回复。";    

}    JsonArray messages = doc.createNestedArray("messages");

    

void DeepSeekClient::addToHistory(const String& userMessage, const String& assistantResponse) {    if (systemPrompt.length() > 0) {

    // 如果历史记录满了，移除最早的一对对话        JsonObject systemMsg = messages.createNestedObject();

    if (historyCount >= MAX_HISTORY * 2) {        systemMsg["role"] = "system";

        cleanOldHistory();        systemMsg["content"] = systemPrompt;

    }    }

        

    // 添加新的对话    JsonObject userMsg = messages.createNestedObject();

    conversationHistory[historyCount] = userMessage;    userMsg["role"] = "user";

    conversationHistory[historyCount + 1] = assistantResponse;    userMsg["content"] = message;

    historyCount += 2;    

}    String payload;

    serializeJson(doc, payload);

void DeepSeekClient::cleanOldHistory() {    

    // 移除最早的4对对话，保留最近的    return payload;

    const int keepPairs = 3;}

    const int keepItems = keepPairs * 2;

    String DeepSeekClient::parseResponse(const String& response) {

    if (historyCount > keepItems) {    DynamicJsonDocument doc(4096);

        int removeItems = historyCount - keepItems;    deserializeJson(doc, response);

            

        // 向前移动数组    if (doc.containsKey("choices") && doc["choices"].size() > 0) {

        for (int i = 0; i < keepItems; i++) {        JsonObject choice = doc["choices"][0];

            conversationHistory[i] = conversationHistory[i + removeItems];        if (choice.containsKey("message") && choice["message"].containsKey("content")) {

        }            return choice["message"]["content"].as<String>();

                }

        historyCount = keepItems;    }

        Serial.printf("[DeepSeek] 清理历史记录，保留%d对对话\n", keepPairs);    

    }    if (doc.containsKey("error")) {

}        JsonObject error = doc["error"];

        String errorMsg = error["message"].as<String>();

String DeepSeekClient::chat(const String& message) {        Serial.println("DeepSeek API错误: " + errorMsg);

    if (!initialized) {        return "抱歉，处理您的请求时出现了错误。";

        lastError = "客户端未初始化";    }

        return "系统未就绪，请稍后再试。";    

    }    return "抱歉，我没有理解您的问题。";

    }

    if (WiFi.status() != WL_CONNECTED) {

        lastError = "WiFi未连接";bool DeepSeekClient::sendRequest(const String& payload) {

        return "网络连接不可用，请检查网络。";    // 发送请求的通用方法

    }    return true;

    }
    Serial.printf("[DeepSeek] 发送消息: %s\n", message.c_str());
    
    HTTPClient http;
    http.begin(client, baseUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.setTimeout(15000); // 15秒超时
    
    String requestBody = buildRequestBody(message);
    
    int httpCode = http.POST(requestBody);
    String response = "";
    
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        String aiResponse = parseResponse(response);
        
        // 添加到历史记录
        addToHistory(message, aiResponse);
        
        Serial.printf("[DeepSeek] AI回复: %s\n", aiResponse.c_str());
        lastError = "";
        http.end();
        return aiResponse;
    } else {
        lastError = "HTTP错误: " + String(httpCode);
        Serial.printf("[DeepSeek] HTTP错误: %d\n", httpCode);
        
        if (httpCode > 0) {
            String errorResponse = http.getString();
            Serial.printf("[DeepSeek] 错误响应: %s\n", errorResponse.c_str());
        }
        
        http.end();
        return "网络请求失败，请稍后再试。";
    }
}

void DeepSeekClient::clearHistory() {
    historyCount = 0;
    Serial.println("[DeepSeek] 对话历史已清除");
}

int DeepSeekClient::getHistoryCount() const {
    return historyCount / 2; // 返回对话对数
}

String DeepSeekClient::getHistoryItem(int index) const {
    if (index * 2 + 1 < historyCount) {
        return "用户: " + conversationHistory[index * 2] + 
               "\n助手: " + conversationHistory[index * 2 + 1];
    }
    return "";
}

void DeepSeekClient::setModel(const String& model) {
    currentModel = model;
}

void DeepSeekClient::setTemperature(float temp) {
    temperature = constrain(temp, 0.0, 2.0);
}

void DeepSeekClient::setMaxTokens(int tokens) {
    maxTokens = constrain(tokens, 1, 4000);
}

bool DeepSeekClient::isConnected() const {
    return initialized && (WiFi.status() == WL_CONNECTED);
}

String DeepSeekClient::getLastError() const {
    return lastError;
}