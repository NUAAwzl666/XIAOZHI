#ifndef BAIDU_REALTIME_ASR_H
#define BAIDU_REALTIME_ASR_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class BaiduRealtimeASR {
public:
    BaiduRealtimeASR();
    ~BaiduRealtimeASR();
    
    // 初始化
    bool begin(const String& appId, const String& appKey);
    
    // 连接和断开
    bool connect();
    void disconnect();
    bool isConnected();
    
    // 发送音频数据
    bool sendAudioData(const uint8_t* data, size_t length);
    
    // 结束识别
    bool finish();
    
    // 取消识别
    bool cancel();
    
    // 循环处理（必须在loop中调用）
    void loop();
    
    // 设置回调函数
    void onPartialResult(void (*callback)(const String& result));
    void onFinalResult(void (*callback)(const String& result, uint32_t startTime, uint32_t endTime));
    void onError(void (*callback)(int errNo, const String& errMsg));
    void onConnected(void (*callback)());
    void onDisconnected(void (*callback)());
    
    // 获取最后的错误信息
    String getLastError();
    
private:
    WebSocketsClient webSocket;
    String appId;
    String appKey;
    String sessionId;
    bool connected;
    String lastError;
    
    // 回调函数指针
    void (*partialResultCallback)(const String& result);
    void (*finalResultCallback)(const String& result, uint32_t startTime, uint32_t endTime);
    void (*errorCallback)(int errNo, const String& errMsg);
    void (*connectedCallback)();
    void (*disconnectedCallback)();
    
    // WebSocket事件处理
    void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    static void webSocketEventStatic(WStype_t type, uint8_t* payload, size_t length);
    static BaiduRealtimeASR* instance;
    
    // 发送开始帧
    bool sendStartFrame();
    
    // 处理接收到的消息
    void handleMessage(const String& message);
    
    // 生成随机会话ID
    String generateSessionId();
};

#endif // BAIDU_REALTIME_ASR_H
