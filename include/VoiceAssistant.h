#ifndef VOICE_ASSISTANT_H
#define VOICE_ASSISTANT_H

#include <Arduino.h>
#include "AudioManager.h"
#include "LEDManager.h"
#include "DeepSeekClient.h"
#include "BaiduSpeech.h"

enum AssistantState {
    STATE_IDLE,
    STATE_LISTENING,
    STATE_PROCESSING,
    STATE_SPEAKING,
    STATE_ERROR
};

class VoiceAssistant {
private:
    AssistantState currentState;
    unsigned long stateStartTime;
    unsigned long lastActivityTime;
    bool isRecording;
    std::vector<int16_t> recordedAudio;
    String lastRecognizedText;
    String lastResponse;
    
    // 语音检测相关
    float silenceThreshold;
    unsigned long silenceStartTime;
    unsigned long recordingStartTime;
    
public:
    VoiceAssistant();
    
    // 初始化方法
    bool initialize();
    
    // 主循环
    void update();
    
    // 状态控制
    void setState(AssistantState newState);
    AssistantState getState();
    
    // 语音交互方法
    void startListening();
    void stopListening();
    void processAudio();
    void speakResponse(const String& text);
    
    // 唤醒词检测
    bool detectWakeWord(const String& text);
    
    // 配置方法
    void setSilenceThreshold(float threshold);
    void setSystemPrompt(const String& prompt);
    
    // 状态查询
    bool isIdle();
    bool isBusy();
    String getLastRecognizedText();
    String getLastResponse();
    
private:
    void handleIdleState();
    void handleListeningState();
    void handleProcessingState();
    void handleSpeakingState();
    void handleErrorState();
    
    bool detectSilence(const std::vector<int16_t>& audioData);
    float calculateRMS(const std::vector<int16_t>& audioData);
    void resetRecording();
    String systemPrompt;
};

extern VoiceAssistant voiceAssistant;

#endif