#ifndef VOICE_ASSISTANT_H
#define VOICE_ASSISTANT_H

#include <Arduino.h>
#include "AudioManager.h"
#include "DeepSeekClient.h"
#include "BaiduSpeech.h"

enum class VoiceState {
    IDLE,           // 空闲状态
    LISTENING,      // 监听唤醒词
    RECORDING,      // 录音中
    PROCESSING,     // 处理中
    SPEAKING,       // 播放回复
    ERROR          // 错误状态
};

class VoiceAssistant {
private:
    AudioManager* audioManager;
    DeepSeekClient* deepSeekClient;
    BaiduSpeech* baiduSpeech;
    
    VoiceState currentState;
    unsigned long stateStartTime;
    
    // 唤醒词检测
    bool detectWakeWord(const String& text);
    String wakeWords[5] = {"小智", "你好", "嗨", "hello", "xiaozhi"};
    
    // 音频录制缓冲区
    static const size_t RECORD_BUFFER_SIZE = 32768; // 32KB
    uint8_t* recordBuffer;
    size_t recordedSize;
    
    // 状态管理
    void setState(VoiceState newState);
    void handleStateTimeout();
    
    // 语音处理流程
    bool processVoiceInput();
    bool synthesizeAndPlay(const String& text);
    
    // 错误处理
    void handleError(const String& error);

public:
    VoiceAssistant();
    ~VoiceAssistant();
    
    // 初始化
    bool begin(AudioManager* audio, DeepSeekClient* deepseek, BaiduSpeech* baidu);
    void end();
    
    // 主循环
    void loop();
    
    // 手动控制
    bool startListening();
    bool stopListening();
    String processTextInput(const String& text); // 文本输入模式
    
    // 状态查询
    VoiceState getState() const;
    String getStateString() const;
    String getStateString(VoiceState state) const;
    bool isReady() const;
    
    // 配置
    void setWakeWords(const String* words, int count);
    void setRecordTimeout(unsigned long timeout);
    void setProcessTimeout(unsigned long timeout);
    
    // 统计信息
    int getConversationCount() const;
    unsigned long getLastActivityTime() const;

private:
    unsigned long recordTimeout;
    unsigned long processTimeout;
    unsigned long lastActivityTime;
    int conversationCount;
    String lastError;
    bool initialized;
};

#endif