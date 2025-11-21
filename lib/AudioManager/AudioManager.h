#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/queue.h>

class AudioManager {
private:
    // I2S配置
    static const i2s_port_t I2S_PORT_MIC = I2S_NUM_0;
    static const i2s_port_t I2S_PORT_SPK = I2S_NUM_1;
    
    // 音频缓冲区
    static const int BUFFER_SIZE = 1024;
    static const int SAMPLE_RATE = 16000;
    static const int BITS_PER_SAMPLE = 16;
    
    uint8_t* audioBuffer;
    size_t bufferSize;
    bool micInitialized;
    bool spkInitialized;
    bool isRecording;
    bool isPlaying;
    
    // 音频数据队列
    QueueHandle_t audioQueue;
    
    // 私有方法
    bool initMicrophone();
    bool initSpeaker();
    void configureI2S();

public:
    AudioManager();
    ~AudioManager();
    
    // 初始化
    bool begin();
    void end();
    
    // 录音功能
    bool startRecording();
    bool stopRecording();
    size_t readAudioData(uint8_t* buffer, size_t maxSize);
    bool isRecordingActive() const;
    
    // 播放功能
    bool startPlayback();
    bool stopPlayback();
    bool playAudioData(const uint8_t* data, size_t size);
    bool isPlaybackActive() const;
    
    // 音频处理
    void setMicrophoneGain(float gain);
    void setSpeakerVolume(float volume);
    void enableNoiseReduction(bool enable);
    
    // 状态检查
    bool isMicrophoneReady() const;
    bool isSpeakerReady() const;
    String getLastError() const;
    
    // 音频格式转换
    void convertTo16Bit(const uint8_t* input, uint8_t* output, size_t samples);
    void amplifyAudio(uint8_t* audio, size_t size, float gain);

private:
    float micGain;
    float spkVolume;
    bool noiseReduction;
    String lastError;
};

#endif