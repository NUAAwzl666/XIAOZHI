#include "BaiduRealtimeASR.h"

// 静态实例指针（用于WebSocket回调）
BaiduRealtimeASR* BaiduRealtimeASR::instance = nullptr;

BaiduRealtimeASR::BaiduRealtimeASR() {
    instance = this;
    connected = false;
    partialResultCallback = nullptr;
    finalResultCallback = nullptr;
    errorCallback = nullptr;
    connectedCallback = nullptr;
    disconnectedCallback = nullptr;
}

BaiduRealtimeASR::~BaiduRealtimeASR() {
    disconnect();
}

bool BaiduRealtimeASR::begin(const String& appId, const String& appKey) {
    this->appId = appId;
    this->appKey = appKey;
    
    Serial.println("[REALTIME-ASR] 初始化实时语音识别");
    Serial.printf("[REALTIME-ASR] AppID: %s\n", appId.c_str());
    
    return true;
}

String BaiduRealtimeASR::generateSessionId() {
    // 生成UUID格式的会话ID: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    String uuid = "";
    const char* hex = "0123456789abcdef";
    
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid += "-";
        } else {
            uuid += hex[random(16)];
        }
    }
    
    return uuid;
}

bool BaiduRealtimeASR::connect() {
    if (connected) {
        Serial.println("[REALTIME-ASR] 已经连接");
        return true;
    }
    
    // 生成会话ID
    sessionId = generateSessionId();
    
    Serial.println("[REALTIME-ASR] 正在连接WebSocket服务器...");
    Serial.printf("[REALTIME-ASR] Session ID: %s\n", sessionId.c_str());
    
    // 配置WebSocket
    // wss://vop.baidu.com/realtime_asr?sn=XXXX-XXXX-XXXX-XXX
    String path = "/realtime_asr?sn=" + sessionId;
    
    webSocket.beginSSL("vop.baidu.com", 443, path);
    webSocket.onEvent(std::bind(&BaiduRealtimeASR::webSocketEventStatic, 
                                std::placeholders::_1, 
                                std::placeholders::_2, 
                                std::placeholders::_3));
    webSocket.setReconnectInterval(0);  // 禁用自动重连，手动管理连接
    
    // 等待连接（最多5秒）
    unsigned long startTime = millis();
    while (!connected && millis() - startTime < 5000) {
        webSocket.loop();
        delay(10);
    }    if (connected) {
        Serial.println("[REALTIME-ASR] ✓ WebSocket连接成功");
        return true;
    } else {
        lastError = "WebSocket连接超时";
        Serial.println("[REALTIME-ASR] ✗ WebSocket连接失败");
        return false;
    }
}

void BaiduRealtimeASR::disconnect() {
    if (connected) {
        Serial.println("[REALTIME-ASR] 断开WebSocket连接");
        webSocket.disconnect();
        connected = false;
    }
}

bool BaiduRealtimeASR::isConnected() {
    return connected;
}

void BaiduRealtimeASR::webSocketEventStatic(WStype_t type, uint8_t* payload, size_t length) {
    if (instance) {
        instance->webSocketEvent(type, payload, length);
    }
}

void BaiduRealtimeASR::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[REALTIME-ASR] WebSocket断开连接");
            connected = false;
            if (disconnectedCallback) {
                disconnectedCallback();
            }
            break;
            
        case WStype_CONNECTED:
            Serial.println("[REALTIME-ASR] WebSocket已连接");
            connected = true;
            
            // 发送开始帧
            if (sendStartFrame()) {
                Serial.println("[REALTIME-ASR] ✓ 开始帧发送成功");
                if (connectedCallback) {
                    connectedCallback();
                }
            } else {
                Serial.println("[REALTIME-ASR] ✗ 开始帧发送失败");
                lastError = "发送开始帧失败";
            }
            break;
            
        case WStype_TEXT:
            // 接收到文本消息（识别结果）
            {
                String message = String((char*)payload);
                handleMessage(message);
            }
            break;
            
        case WStype_BIN:
            // 不应该收到二进制消息
            Serial.println("[REALTIME-ASR] ⚠️ 收到意外的二进制消息");
            break;
            
        case WStype_ERROR:
            Serial.printf("[REALTIME-ASR] ✗ WebSocket错误: %s\n", payload);
            lastError = String((char*)payload);
            if (errorCallback) {
                errorCallback(-1, lastError);
            }
            break;
            
        case WStype_PING:
            Serial.println("[REALTIME-ASR] 收到PING");
            break;
            
        case WStype_PONG:
            Serial.println("[REALTIME-ASR] 收到PONG");
            break;
    }
}

bool BaiduRealtimeASR::sendStartFrame() {
    Serial.println("[REALTIME-ASR] 发送开始帧...");
    
    DynamicJsonDocument doc(1024);
    doc["type"] = "START";
    
    JsonObject data = doc.createNestedObject("data");
    data["appid"] = appId.toInt();
    data["appkey"] = appKey;
    data["dev_pid"] = 15372;  // 中文普通话，加强标点
    data["cuid"] = "esp32_s3_voice_assistant";
    data["format"] = "pcm";
    data["sample"] = 16000;
    
    String json;
    serializeJson(doc, json);
    
    Serial.printf("[REALTIME-ASR] 开始帧: %s\n", json.c_str());
    Serial.printf("[REALTIME-ASR] JSON大小: %d bytes\n", json.length());
    
    bool result = webSocket.sendTXT(json);
    
    if (result) {
        Serial.println("[REALTIME-ASR] ✓ 开始帧发送成功");
    } else {
        Serial.println("[REALTIME-ASR] ✗ 开始帧发送失败");
    }
    
    return result;
}

bool BaiduRealtimeASR::sendAudioData(const uint8_t* data, size_t length) {
    if (!connected) {
        static unsigned long lastWarnTime = 0;
        if (millis() - lastWarnTime > 1000) {
            Serial.println("[REALTIME-ASR] ✗ 未连接，无法发送音频数据");
            lastWarnTime = millis();
        }
        return false;
    }
    
    // 发送二进制音频数据帧
    bool result = webSocket.sendBIN(data, length);
    
    // 定期输出发送统计
    static unsigned long totalSent = 0;
    static unsigned long lastReportTime = 0;
    totalSent += length;
    
    if (millis() - lastReportTime > 2000) {
        Serial.printf("[REALTIME-ASR] 已发送 %lu bytes 音频数据\n", totalSent);
        lastReportTime = millis();
    }
    
    if (!result) {
        Serial.printf("[REALTIME-ASR] ✗ 音频数据发送失败 (%d bytes)\n", length);
    }
    
    return result;
}

bool BaiduRealtimeASR::finish() {
    if (!connected) {
        Serial.println("[REALTIME-ASR] ✗ 未连接，无法发送结束帧");
        return false;
    }
    
    Serial.println("[REALTIME-ASR] 发送结束帧...");
    
    DynamicJsonDocument doc(128);
    doc["type"] = "FINISH";
    
    String json;
    serializeJson(doc, json);
    
    bool result = webSocket.sendTXT(json);
    
    if (result) {
        Serial.println("[REALTIME-ASR] ✓ 结束帧发送成功");
    } else {
        Serial.println("[REALTIME-ASR] ✗ 结束帧发送失败");
    }
    
    return result;
}

bool BaiduRealtimeASR::cancel() {
    if (!connected) {
        Serial.println("[REALTIME-ASR] ✗ 未连接，无法发送取消帧");
        return false;
    }
    
    Serial.println("[REALTIME-ASR] 发送取消帧...");
    
    DynamicJsonDocument doc(128);
    doc["type"] = "CANCEL";
    
    String json;
    serializeJson(doc, json);
    
    bool result = webSocket.sendTXT(json);
    
    if (result) {
        Serial.println("[REALTIME-ASR] ✓ 取消帧发送成功");
    } else {
        Serial.println("[REALTIME-ASR] ✗ 取消帧发送失败");
    }
    
    return result;
}

void BaiduRealtimeASR::loop() {
    webSocket.loop();
}

void BaiduRealtimeASR::handleMessage(const String& message) {
    Serial.printf("[REALTIME-ASR] 收到消息: %s\n", message.c_str());
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.printf("[REALTIME-ASR] ✗ JSON解析失败: %s\n", error.c_str());
        return;
    }
    
    String type = doc["type"].as<String>();
    
    // 心跳帧，忽略
    if (type == "HEARTBEAT") {
        return;
    }
    
    int errNo = doc["err_no"].as<int>();
    String errMsg = doc["err_msg"].as<String>();
    
    // 检查错误
    if (errNo != 0) {
        Serial.printf("[REALTIME-ASR] ✗ 错误 %d: %s\n", errNo, errMsg.c_str());
        lastError = errMsg;
        if (errorCallback) {
            errorCallback(errNo, errMsg);
        }
        return;
    }
    
    String result = doc["result"].as<String>();
    
    if (type == "MID_TEXT") {
        // 临时识别结果
        Serial.printf("[REALTIME-ASR] 临时结果: %s\n", result.c_str());
        if (partialResultCallback) {
            partialResultCallback(result);
        }
    } else if (type == "FIN_TEXT") {
        // 最终识别结果
        uint32_t startTime = doc["start_time"].as<uint32_t>();
        uint32_t endTime = doc["end_time"].as<uint32_t>();
        
        Serial.printf("[REALTIME-ASR] ✓ 最终结果: %s [%d-%d ms]\n", 
                      result.c_str(), startTime, endTime);
        
        if (finalResultCallback) {
            finalResultCallback(result, startTime, endTime);
        }
    }
}

void BaiduRealtimeASR::onPartialResult(void (*callback)(const String& result)) {
    partialResultCallback = callback;
}

void BaiduRealtimeASR::onFinalResult(void (*callback)(const String& result, uint32_t startTime, uint32_t endTime)) {
    finalResultCallback = callback;
}

void BaiduRealtimeASR::onError(void (*callback)(int errNo, const String& errMsg)) {
    errorCallback = callback;
}

void BaiduRealtimeASR::onConnected(void (*callback)()) {
    connectedCallback = callback;
}

void BaiduRealtimeASR::onDisconnected(void (*callback)()) {
    disconnectedCallback = callback;
}

String BaiduRealtimeASR::getLastError() {
    return lastError;
}
