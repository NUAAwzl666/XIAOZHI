#include "AudioManager.h"#include "AudioManager.h"

#include "config.h"

AudioManager::AudioManager() {

    audioBuffer = nullptr;AudioManager audioManager;

    bufferSize = BUFFER_SIZE;

    micInitialized = false;AudioManager::AudioManager() {

    spkInitialized = false;    audioBuffer.reserve(BUFFER_SIZE);

    isRecording = false;}

    isPlaying = false;

    micGain = 1.0;AudioManager::~AudioManager() {

    spkVolume = 0.8;    deinitialize();

    noiseReduction = false;}

    lastError = "";

    audioQueue = nullptr;bool AudioManager::initMicrophone() {

}    if (micInitialized) {

        return true;

AudioManager::~AudioManager() {    }

    end();    

}    Serial.println("初始化麦克风...");

    

bool AudioManager::begin() {    i2s_config_t i2s_config_mic = {

    Serial.println("[AudioManager] 初始化音频管理器...");        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),

            .sample_rate = SAMPLE_RATE,

    // 分配音频缓冲区        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,

    audioBuffer = (uint8_t*)malloc(bufferSize);        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,

    if (!audioBuffer) {        .communication_format = I2S_COMM_FORMAT_STAND_I2S,

        lastError = "音频缓冲区分配失败";        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,

        return false;        .dma_buf_count = 4,

    }        .dma_buf_len = BUFFER_SIZE,

            .use_apll = false,

    // 创建音频队列        .tx_desc_auto_clear = false,

    audioQueue = xQueueCreate(10, sizeof(size_t));        .fixed_mclk = 0

    if (!audioQueue) {    };

        lastError = "音频队列创建失败";    

        free(audioBuffer);    i2s_pin_config_t pin_config_mic = {

        return false;        .bck_io_num = I2S_MIC_SCK_PIN,

    }        .ws_io_num = I2S_MIC_WS_PIN,

            .data_out_num = I2S_PIN_NO_CHANGE,

    // 初始化麦克风        .data_in_num = I2S_MIC_SD_PIN

    if (!initMicrophone()) {    };

        end();    

        return false;    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config_mic, 0, NULL);

    }    if (err != ESP_OK) {

            Serial.printf("麦克风I2S驱动安装失败: %s\n", esp_err_to_name(err));

    // 初始化扬声器        return false;

    if (!initSpeaker()) {    }

        end();    

        return false;    err = i2s_set_pin(I2S_NUM_0, &pin_config_mic);

    }    if (err != ESP_OK) {

            Serial.printf("麦克风引脚配置失败: %s\n", esp_err_to_name(err));

    Serial.println("[AudioManager] 音频管理器初始化成功");        i2s_driver_uninstall(I2S_NUM_0);

    return true;        return false;

}    }

    

void AudioManager::end() {    micInitialized = true;

    stopRecording();    Serial.println("麦克风初始化成功");

    stopPlayback();    return true;

    }

    if (micInitialized) {

        i2s_driver_uninstall(I2S_PORT_MIC);bool AudioManager::initSpeaker() {

        micInitialized = false;    if (speakerInitialized) {

    }        return true;

        }

    if (spkInitialized) {    

        i2s_driver_uninstall(I2S_PORT_SPK);    Serial.println("初始化扬声器...");

        spkInitialized = false;    

    }    i2s_config_t i2s_config_spk = {

            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),

    if (audioBuffer) {        .sample_rate = SAMPLE_RATE,

        free(audioBuffer);        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,

        audioBuffer = nullptr;        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,

    }        .communication_format = I2S_COMM_FORMAT_STAND_I2S,

            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,

    if (audioQueue) {        .dma_buf_count = 4,

        vQueueDelete(audioQueue);        .dma_buf_len = BUFFER_SIZE,

        audioQueue = nullptr;        .use_apll = false,

    }        .tx_desc_auto_clear = true,

            .fixed_mclk = 0

    Serial.println("[AudioManager] 音频管理器已关闭");    };

}    

    i2s_pin_config_t pin_config_spk = {

bool AudioManager::initMicrophone() {        .bck_io_num = I2S_SPK_SCK_PIN,

    Serial.println("[AudioManager] 初始化麦克风...");        .ws_io_num = I2S_SPK_WS_PIN,

            .data_out_num = I2S_SPK_SD_PIN,

    i2s_config_t i2s_config = {        .data_in_num = I2S_PIN_NO_CHANGE

        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),    };

        .sample_rate = SAMPLE_RATE,    

        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,    esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2s_config_spk, 0, NULL);

        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,    if (err != ESP_OK) {

        .communication_format = I2S_COMM_FORMAT_STAND_I2S,        Serial.printf("扬声器I2S驱动安装失败: %s\n", esp_err_to_name(err));

        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,        return false;

        .dma_buf_count = 8,    }

        .dma_buf_len = 256,    

        .use_apll = false,    err = i2s_set_pin(I2S_NUM_1, &pin_config_spk);

        .tx_desc_auto_clear = false,    if (err != ESP_OK) {

        .fixed_mclk = 0        Serial.printf("扬声器引脚配置失败: %s\n", esp_err_to_name(err));

    };        i2s_driver_uninstall(I2S_NUM_1);

            return false;

    esp_err_t result = i2s_driver_install(I2S_PORT_MIC, &i2s_config, 0, NULL);    }

    if (result != ESP_OK) {    

        lastError = "麦克风I2S驱动安装失败: " + String(result);    speakerInitialized = true;

        return false;    Serial.println("扬声器初始化成功");

    }    return true;

    }

    i2s_pin_config_t pin_config = {

        .bck_io_num = 21,    // SCKvoid AudioManager::deinitialize() {

        .ws_io_num = 20,     // WS (LRCLK)    if (micInitialized) {

        .data_out_num = I2S_PIN_NO_CHANGE,        i2s_driver_uninstall(I2S_NUM_0);

        .data_in_num = 19    // SD (DIN)        micInitialized = false;

    };    }

        

    result = i2s_set_pin(I2S_PORT_MIC, &pin_config);    if (speakerInitialized) {

    if (result != ESP_OK) {        i2s_driver_uninstall(I2S_NUM_1);

        lastError = "麦克风引脚配置失败: " + String(result);        speakerInitialized = false;

        i2s_driver_uninstall(I2S_PORT_MIC);    }

        return false;}

    }

    bool AudioManager::startRecording() {

    micInitialized = true;    if (!micInitialized) {

    Serial.println("[AudioManager] 麦克风初始化成功");        if (!initMicrophone()) {

    return true;            return false;

}        }

    }

bool AudioManager::initSpeaker() {    

    Serial.println("[AudioManager] 初始化扬声器...");    audioBuffer.clear();

        i2s_start(I2S_NUM_0);

    i2s_config_t i2s_config = {    Serial.println("开始录音");

        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),    return true;

        .sample_rate = SAMPLE_RATE,}

        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,

        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,void AudioManager::stopRecording() {

        .communication_format = I2S_COMM_FORMAT_STAND_I2S,    if (micInitialized) {

        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,        i2s_stop(I2S_NUM_0);

        .dma_buf_count = 8,        Serial.println("停止录音");

        .dma_buf_len = 256,    }

        .use_apll = false,}

        .tx_desc_auto_clear = true,

        .fixed_mclk = 0std::vector<int16_t> AudioManager::getAudioData() {

    };    if (!micInitialized) {

            return std::vector<int16_t>();

    esp_err_t result = i2s_driver_install(I2S_PORT_SPK, &i2s_config, 0, NULL);    }

    if (result != ESP_OK) {    

        lastError = "扬声器I2S驱动安装失败: " + String(result);    int16_t buffer[BUFFER_SIZE];

        return false;    size_t bytesRead = 0;

    }    

        esp_err_t err = i2s_read(I2S_NUM_0, buffer, sizeof(buffer), &bytesRead, pdMS_TO_TICKS(100));

    i2s_pin_config_t pin_config = {    if (err == ESP_OK && bytesRead > 0) {

        .bck_io_num = 41,    // BCLK        size_t samplesRead = bytesRead / sizeof(int16_t);

        .ws_io_num = 42,     // LRCLK        std::vector<int16_t> audioData(buffer, buffer + samplesRead);

        .data_out_num = 40,  // DIN        

        .data_in_num = I2S_PIN_NO_CHANGE        // 添加到缓冲区

    };        audioBuffer.insert(audioBuffer.end(), audioData.begin(), audioData.end());

            

    result = i2s_set_pin(I2S_PORT_SPK, &pin_config);        return audioData;

    if (result != ESP_OK) {    }

        lastError = "扬声器引脚配置失败: " + String(result);    

        i2s_driver_uninstall(I2S_PORT_SPK);    return std::vector<int16_t>();

        return false;}

    }

    bool AudioManager::isRecording() {

    spkInitialized = true;    return micInitialized;

    Serial.println("[AudioManager] 扬声器初始化成功");}

    return true;

}bool AudioManager::playAudio(const uint8_t* audioData, size_t dataSize) {

    if (!speakerInitialized) {

bool AudioManager::startRecording() {        if (!initSpeaker()) {

    if (!micInitialized) {            return false;

        lastError = "麦克风未初始化";        }

        return false;    }

    }    

        size_t bytesWritten = 0;

    if (isRecording) {    esp_err_t err = i2s_write(I2S_NUM_1, audioData, dataSize, &bytesWritten, pdMS_TO_TICKS(1000));

        Serial.println("[AudioManager] 已在录音中");    

        return true;    if (err != ESP_OK) {

    }        Serial.printf("音频播放失败: %s\n", esp_err_to_name(err));

            return false;

    // 清空I2S缓冲区    }

    i2s_zero_dma_buffer(I2S_PORT_MIC);    

        return bytesWritten == dataSize;

    isRecording = true;}

    Serial.println("[AudioManager] 开始录音");

    return true;bool AudioManager::playAudioFromBuffer(const std::vector<int16_t>& buffer) {

}    if (buffer.empty()) {

        return false;

bool AudioManager::stopRecording() {    }

    if (!isRecording) {    

        return true;    return playAudio((const uint8_t*)buffer.data(), buffer.size() * sizeof(int16_t));

    }}

    

    isRecording = false;void AudioManager::setVolume(float volume) {

    Serial.println("[AudioManager] 停止录音");    // 音量控制逻辑可以在这里实现

    return true;    // 对于98357功放，可以通过调整音频数据的振幅来控制音量

}}



size_t AudioManager::readAudioData(uint8_t* buffer, size_t maxSize) {bool AudioManager::isMicrophoneReady() {

    if (!micInitialized || !isRecording) {    return micInitialized;

        return 0;}

    }

    bool AudioManager::isSpeakerReady() {

    size_t bytesRead = 0;    return speakerInitialized;

    esp_err_t result = i2s_read(I2S_PORT_MIC, buffer, maxSize, &bytesRead, portMAX_DELAY);}
    
    if (result == ESP_OK && bytesRead > 0) {
        // 应用增益
        if (micGain != 1.0) {
            amplifyAudio(buffer, bytesRead, micGain);
        }
        
        // 简单的噪声抑制
        if (noiseReduction) {
            // TODO: 实现更复杂的噪声抑制算法
            // 目前只是简单的阈值处理
            int16_t* samples = (int16_t*)buffer;
            size_t sampleCount = bytesRead / 2;
            const int16_t threshold = 500;
            
            for (size_t i = 0; i < sampleCount; i++) {
                if (abs(samples[i]) < threshold) {
                    samples[i] = 0;
                }
            }
        }
        
        return bytesRead;
    }
    
    return 0;
}

bool AudioManager::startPlayback() {
    if (!spkInitialized) {
        lastError = "扬声器未初始化";
        return false;
    }
    
    if (isPlaying) {
        Serial.println("[AudioManager] 已在播放中");
        return true;
    }
    
    // 清空I2S缓冲区
    i2s_zero_dma_buffer(I2S_PORT_SPK);
    
    isPlaying = true;
    Serial.println("[AudioManager] 开始播放");
    return true;
}

bool AudioManager::stopPlayback() {
    if (!isPlaying) {
        return true;
    }
    
    isPlaying = false;
    Serial.println("[AudioManager] 停止播放");
    return true;
}

bool AudioManager::playAudioData(const uint8_t* data, size_t size) {
    if (!spkInitialized || !isPlaying) {
        lastError = "扬声器未就绪或未开始播放";
        return false;
    }
    
    // 复制数据到临时缓冲区以应用音量
    uint8_t* tempBuffer = (uint8_t*)malloc(size);
    if (!tempBuffer) {
        lastError = "音频播放缓冲区分配失败";
        return false;
    }
    
    memcpy(tempBuffer, data, size);
    
    // 应用音量控制
    if (spkVolume != 1.0) {
        amplifyAudio(tempBuffer, size, spkVolume);
    }
    
    size_t bytesWritten = 0;
    esp_err_t result = i2s_write(I2S_PORT_SPK, tempBuffer, size, &bytesWritten, portMAX_DELAY);
    
    free(tempBuffer);
    
    if (result == ESP_OK) {
        return bytesWritten == size;
    } else {
        lastError = "I2S写入失败: " + String(result);
        return false;
    }
}

void AudioManager::setMicrophoneGain(float gain) {
    micGain = constrain(gain, 0.1, 5.0);
    Serial.printf("[AudioManager] 麦克风增益设置为: %.2f\n", micGain);
}

void AudioManager::setSpeakerVolume(float volume) {
    spkVolume = constrain(volume, 0.0, 1.0);
    Serial.printf("[AudioManager] 扬声器音量设置为: %.2f\n", spkVolume);
}

void AudioManager::enableNoiseReduction(bool enable) {
    noiseReduction = enable;
    Serial.printf("[AudioManager] 噪声抑制: %s\n", enable ? "开启" : "关闭");
}

void AudioManager::amplifyAudio(uint8_t* audio, size_t size, float gain) {
    int16_t* samples = (int16_t*)audio;
    size_t sampleCount = size / 2;
    
    for (size_t i = 0; i < sampleCount; i++) {
        int32_t amplified = (int32_t)(samples[i] * gain);
        // 防止溢出
        if (amplified > 32767) amplified = 32767;
        if (amplified < -32768) amplified = -32768;
        samples[i] = (int16_t)amplified;
    }
}

bool AudioManager::isRecordingActive() const {
    return isRecording && micInitialized;
}

bool AudioManager::isPlaybackActive() const {
    return isPlaying && spkInitialized;
}

bool AudioManager::isMicrophoneReady() const {
    return micInitialized;
}

bool AudioManager::isSpeakerReady() const {
    return spkInitialized;
}

String AudioManager::getLastError() const {
    return lastError;
}