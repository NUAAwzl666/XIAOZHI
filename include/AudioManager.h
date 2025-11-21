#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <vector>

class AudioManager {
private:
    bool micInitialized = false;
    bool speakerInitialized = false;
    std::vector<int16_t> audioBuffer;
    
public:
    AudioManager();
    ~AudioManager();
    
    // 初始化方法
    bool initMicrophone();
    bool initSpeaker();
    void deinitialize();
    
    // 录音方法
    bool startRecording();
    void stopRecording();
    std::vector<int16_t> getAudioData();
    bool isRecording();
    
    // 播放方法
    bool playAudio(const uint8_t* audioData, size_t dataSize);
    bool playAudioFromBuffer(const std::vector<int16_t>& buffer);
    
    // 音量控制
    void setVolume(float volume);
    
    // 状态检查
    bool isMicrophoneReady();
    bool isSpeakerReady();
    
private:
    void setupI2SMicrophone();
    void setupI2SSpeaker();
    size_t readAudioData(void* buffer, size_t size);
    size_t writeAudioData(const void* buffer, size_t size);
};

extern AudioManager audioManager;

#endif