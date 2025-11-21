#ifndef BAIDU_SPEECH_H
#define BAIDU_SPEECH_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <functional>

class BaiduSpeech {
private:
    String appId;
    String apiKey;
    String secretKey;
    String accessToken;
    unsigned long tokenExpireTime;
    // 移除持久的client对象，改为临时创建
    
    // API URLs
    String tokenUrl;
    String asrUrl;   // 语音识别
    String ttsUrl;   // 语音合成
    
    // ASR配置
    String asrLanguage;
    int asrSampleRate;
    
    // TTS配置
    String ttsVoice;
    int ttsSpeed;
    int ttsPitch;
    int ttsVolume;
    
    String lastError;
    bool initialized;
    
    // 私有方法
    bool getAccessToken();
    String base64Encode(const uint8_t* data, size_t length);
    bool isTokenValid();
    String urlEncode(const String& value);
    String sanitizeTTS(const String& text);

public:
    BaiduSpeech();
    ~BaiduSpeech();
    
    // 初始化
    bool begin(const String& appId, const String& apiKey, const String& secretKey);
    
    // 语音识别 (ASR - Automatic Speech Recognition)
    String recognizeSpeech(const uint8_t* audioData, size_t dataSize, const String& format = "pcm");
    String recognizeSpeechRaw(const uint8_t* audioData, size_t dataSize, int sampleRate = 16000); // RAW格式，节省内存
    String recognizeSpeechFromFile(const String& filePath);
    
    // 语音合成 (TTS - Text To Speech)
    bool synthesizeSpeech(const String& text, uint8_t** audioData, size_t* dataSize);
    bool synthesizeSpeechToFile(const String& text, const String& filePath);
    // 流式语音合成（边下边播）
    bool synthesizeSpeechStream(const String& text,
                                std::function<bool(const uint8_t* chunk, size_t len)> onAudioChunk,
                                int sampleRate = 16000,
                                int aue = 6);
    
    // 配置方法
    void setASRConfig(const String& language = "zh", int sampleRate = 16000);
    void setTTSConfig(const String& voice = "zh", int speed = 5, int pitch = 5, int volume = 5);
    
    // 状态检查
    bool isInitialized() const;
    String getLastError() const;
};

#endif