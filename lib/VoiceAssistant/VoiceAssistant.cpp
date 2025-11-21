#include "VoiceAssistant.h"#include "VoiceAssistant.h"

#include "config.h"

VoiceAssistant::VoiceAssistant() {

    audioManager = nullptr;VoiceAssistant voiceAssistant;

    deepSeekClient = nullptr;

    baiduSpeech = nullptr;VoiceAssistant::VoiceAssistant() : 

        currentState(STATE_IDLE),

    currentState = VoiceState::IDLE;    stateStartTime(0),

    stateStartTime = 0;    lastActivityTime(0),

    recordBuffer = nullptr;    isRecording(false),

    recordedSize = 0;    silenceThreshold(0.01),

        silenceStartTime(0),

    recordTimeout = 10000;  // 10秒录音超时    recordingStartTime(0) {

    processTimeout = 30000; // 30秒处理超时    

    lastActivityTime = 0;    systemPrompt = "你是小智，一个友好的智能语音助手。请用简洁、自然的中文回答用户的问题。回答要简短明了，适合语音播放。";

    conversationCount = 0;}

    initialized = false;

}bool VoiceAssistant::initialize() {

    Serial.println("初始化语音助手...");

VoiceAssistant::~VoiceAssistant() {    

    end();    // 初始化硬件模块

}    if (!audioManager.initMicrophone()) {

        Serial.println("麦克风初始化失败");

bool VoiceAssistant::begin(AudioManager* audio, DeepSeekClient* deepseek, BaiduSpeech* baidu) {        return false;

    if (!audio || !deepseek || !baidu) {    }

        lastError = "参数不能为空";    

        return false;    if (!audioManager.initSpeaker()) {

    }        Serial.println("扬声器初始化失败");

            return false;

    audioManager = audio;    }

    deepSeekClient = deepseek;    

    baiduSpeech = baidu;    if (!ledManager.initialize()) {

            Serial.println("LED初始化失败");

    // 分配录音缓冲区        return false;

    recordBuffer = (uint8_t*)malloc(RECORD_BUFFER_SIZE);    }

    if (!recordBuffer) {    

        lastError = "录音缓冲区分配失败";    // 初始化AI服务

        return false;    if (!deepSeekClient.initialize(DEEPSEEK_API_KEY, DEEPSEEK_BASE_URL)) {

    }        Serial.println("DeepSeek客户端初始化失败");

            return false;

    // 检查各组件状态    }

    if (!audioManager->isMicrophoneReady()) {    

        lastError = "麦克风未就绪";    if (!baiduSpeech.initialize(BAIDU_APP_ID, BAIDU_API_KEY, BAIDU_SECRET_KEY)) {

        free(recordBuffer);        Serial.println("百度语音服务初始化失败");

        return false;        return false;

    }    }

        

    if (!audioManager->isSpeakerReady()) {    setState(STATE_IDLE);

        lastError = "扬声器未就绪";    Serial.println("语音助手初始化成功");

        free(recordBuffer);    return true;

        return false;}

    }

    void VoiceAssistant::update() {

    if (!deepSeekClient->isConnected()) {    unsigned long currentTime = millis();

        lastError = "DeepSeek服务未连接";    

        free(recordBuffer);    // 更新LED状态

        return false;    ledManager.update();

    }    

        // 根据当前状态处理逻辑

    if (!baiduSpeech->isInitialized()) {    switch (currentState) {

        lastError = "百度语音服务未初始化";        case STATE_IDLE:

        free(recordBuffer);            handleIdleState();

        return false;            break;

    }        case STATE_LISTENING:

                handleListeningState();

    setState(VoiceState::LISTENING);            break;

    initialized = true;        case STATE_PROCESSING:

                handleProcessingState();

    Serial.println("[VoiceAssistant] 语音助手初始化成功");            break;

    Serial.println("[VoiceAssistant] 等待唤醒词: 小智、你好、嗨...");        case STATE_SPEAKING:

                handleSpeakingState();

    return true;            break;

}        case STATE_ERROR:

            handleErrorState();

void VoiceAssistant::end() {            break;

    if (recordBuffer) {    }

        free(recordBuffer);    

        recordBuffer = nullptr;    lastActivityTime = currentTime;

    }}

    

    setState(VoiceState::IDLE);void VoiceAssistant::setState(AssistantState newState) {

    initialized = false;    if (currentState != newState) {

            Serial.printf("状态切换: %d -> %d\n", currentState, newState);

    Serial.println("[VoiceAssistant] 语音助手已关闭");        

}        currentState = newState;

        stateStartTime = millis();

void VoiceAssistant::setState(VoiceState newState) {        

    if (currentState != newState) {        // 更新LED状态

        Serial.printf("[VoiceAssistant] 状态变更: %s -> %s\n",         switch (newState) {

                     getStateString().c_str(),             case STATE_IDLE:

                     getStateString(newState).c_str());                ledManager.setState(LED_OFF);

                        break;

        currentState = newState;            case STATE_LISTENING:

        stateStartTime = millis();                ledManager.setState(LED_LISTENING);

                        break;

        // 状态变更时的操作            case STATE_PROCESSING:

        switch (newState) {                ledManager.setState(LED_PROCESSING);

            case VoiceState::LISTENING:                break;

                audioManager->startRecording();            case STATE_SPEAKING:

                break;                ledManager.setState(LED_SPEAKING);

                                break;

            case VoiceState::RECORDING:            case STATE_ERROR:

                recordedSize = 0;                ledManager.setState(LED_ERROR);

                break;                break;

                        }

            case VoiceState::PROCESSING:    }

                audioManager->stopRecording();}

                break;

                AssistantState VoiceAssistant::getState() {

            case VoiceState::SPEAKING:    return currentState;

                audioManager->startPlayback();}

                break;

                void VoiceAssistant::startListening() {

            case VoiceState::IDLE:    if (currentState == STATE_IDLE) {

                audioManager->stopRecording();        Serial.println("开始监听...");

                audioManager->stopPlayback();        resetRecording();

                break;        

                        if (audioManager.startRecording()) {

            case VoiceState::ERROR:            setState(STATE_LISTENING);

                audioManager->stopRecording();            isRecording = true;

                audioManager->stopPlayback();            recordingStartTime = millis();

                break;        } else {

        }            setState(STATE_ERROR);

    }        }

}    }

}

void VoiceAssistant::loop() {

    if (!initialized) {void VoiceAssistant::stopListening() {

        return;    if (isRecording) {

    }        Serial.println("停止监听...");

            audioManager.stopRecording();

    // 检查状态超时        isRecording = false;

    handleStateTimeout();        

            if (!recordedAudio.empty()) {

    switch (currentState) {            processAudio();

        case VoiceState::LISTENING:        } else {

            {            setState(STATE_IDLE);

                // 监听唤醒词        }

                uint8_t buffer[512];    }

                size_t bytesRead = audioManager->readAudioData(buffer, sizeof(buffer));}

                

                if (bytesRead > 0) {void VoiceAssistant::processAudio() {

                    // 简单的音量检测作为唤醒    if (recordedAudio.empty()) {

                    int16_t* samples = (int16_t*)buffer;        setState(STATE_IDLE);

                    size_t sampleCount = bytesRead / 2;        return;

                    int32_t rms = 0;    }

                        

                    for (size_t i = 0; i < sampleCount; i++) {    setState(STATE_PROCESSING);

                        rms += abs(samples[i]);    

                    }    Serial.printf("处理音频数据，长度: %d样本\n", recordedAudio.size());

                    rms /= sampleCount;    

                        // 语音识别

                    // 如果音量超过阈值，开始录音    String recognizedText = baiduSpeech.speechToText(recordedAudio);

                    if (rms > 1000) {    lastRecognizedText = recognizedText;

                        Serial.printf("[VoiceAssistant] 检测到声音，RMS: %d\n", rms);    

                        setState(VoiceState::RECORDING);    if (recognizedText.length() > 0) {

                    }        Serial.println("识别到文本: " + recognizedText);

                }        

            }        // 检查唤醒词

            break;        if (detectWakeWord(recognizedText)) {

                        Serial.println("检测到唤醒词，开始对话");

        case VoiceState::RECORDING:            

            {            // 获取AI响应

                // 录制音频            String response = deepSeekClient.chat(recognizedText, systemPrompt);

                uint8_t buffer[512];            lastResponse = response;

                size_t bytesRead = audioManager->readAudioData(buffer, sizeof(buffer));            

                            if (response.length() > 0) {

                if (bytesRead > 0 && recordedSize + bytesRead < RECORD_BUFFER_SIZE) {                speakResponse(response);

                    memcpy(recordBuffer + recordedSize, buffer, bytesRead);            } else {

                    recordedSize += bytesRead;                setState(STATE_ERROR);

                                }

                    // 检测静音（简单的结束录音条件）        } else {

                    int16_t* samples = (int16_t*)buffer;            Serial.println("未检测到唤醒词，返回待机状态");

                    size_t sampleCount = bytesRead / 2;            setState(STATE_IDLE);

                    int32_t rms = 0;        }

                        } else {

                    for (size_t i = 0; i < sampleCount; i++) {        Serial.println("语音识别失败");

                        rms += abs(samples[i]);        setState(STATE_IDLE);

                    }    }

                    rms /= sampleCount;}

                    

                    // 如果连续静音，结束录音void VoiceAssistant::speakResponse(const String& text) {

                    if (rms < 200) {    setState(STATE_SPEAKING);

                        static int silentFrames = 0;    

                        silentFrames++;    Serial.println("开始语音合成: " + text);

                        if (silentFrames > 20) { // 大约1秒的静音    

                            silentFrames = 0;    // 语音合成

                            setState(VoiceState::PROCESSING);    std::vector<uint8_t> audioData = baiduSpeech.textToSpeech(text);

                        }    

                    } else {    if (!audioData.empty()) {

                        // 重置静音计数        Serial.println("播放语音响应");

                        static int silentFrames = 0;        

                        silentFrames = 0;        // 播放音频

                    }        if (audioManager.playAudio(audioData.data(), audioData.size())) {

                }            Serial.println("语音播放完成");

            }        } else {

            break;            Serial.println("语音播放失败");

                    }

        case VoiceState::PROCESSING:    } else {

            {        Serial.println("语音合成失败");

                if (processVoiceInput()) {    }

                    setState(VoiceState::LISTENING);    

                } else {    setState(STATE_IDLE);

                    setState(VoiceState::ERROR);}

                }

            }bool VoiceAssistant::detectWakeWord(const String& text) {

            break;    // 简单的唤醒词检测

                String lowerText = text;

        case VoiceState::ERROR:    lowerText.toLowerCase();

            {    

                // 错误恢复    return lowerText.indexOf(WAKE_WORD) >= 0 || 

                delay(1000);           lowerText.indexOf("小智") >= 0 ||

                setState(VoiceState::LISTENING);           lowerText.indexOf("你好") >= 0;

            }}

            break;

            void VoiceAssistant::setSilenceThreshold(float threshold) {

        default:    silenceThreshold = threshold;

            break;}

    }

}void VoiceAssistant::setSystemPrompt(const String& prompt) {

    systemPrompt = prompt;

bool VoiceAssistant::processVoiceInput() {}

    if (recordedSize == 0) {

        Serial.println("[VoiceAssistant] 没有录制到音频数据");bool VoiceAssistant::isIdle() {

        return false;    return currentState == STATE_IDLE;

    }}

    

    Serial.printf("[VoiceAssistant] 处理录音数据，大小: %d bytes\n", recordedSize);bool VoiceAssistant::isBusy() {

        return currentState != STATE_IDLE;

    // 1. 语音识别}

    String recognizedText = baiduSpeech->recognizeSpeech(recordBuffer, recordedSize, "pcm");

    String VoiceAssistant::getLastRecognizedText() {

    if (recognizedText.length() == 0) {    return lastRecognizedText;

        Serial.println("[VoiceAssistant] 语音识别失败");}

        handleError("语音识别失败");

        return false;String VoiceAssistant::getLastResponse() {

    }    return lastResponse;

    }

    Serial.printf("[VoiceAssistant] 识别结果: %s\n", recognizedText.c_str());

    void VoiceAssistant::handleIdleState() {

    // 2. 检查是否为唤醒词（如果当前是监听状态）    // 在空闲状态下持续监听音频，检测唤醒词

    if (currentState == VoiceState::PROCESSING && detectWakeWord(recognizedText)) {    if (audioManager.isMicrophoneReady()) {

        Serial.println("[VoiceAssistant] 检测到唤醒词，等待指令...");        std::vector<int16_t> audioData = audioManager.getAudioData();

        synthesizeAndPlay("我在，请说");        

        return true;        if (!audioData.empty()) {

    }            float rms = calculateRMS(audioData);

                

    // 3. AI对话            // 如果检测到声音超过阈值，开始录音

    String aiResponse = deepSeekClient->chat(recognizedText);            if (rms > silenceThreshold) {

                    startListening();

    if (aiResponse.length() == 0) {                

        Serial.println("[VoiceAssistant] AI对话失败");                // 将当前音频数据添加到录音缓冲区

        handleError("AI服务暂时不可用");                recordedAudio.insert(recordedAudio.end(), audioData.begin(), audioData.end());

        return false;            }

    }        }

        }

    Serial.printf("[VoiceAssistant] AI回复: %s\n", aiResponse.c_str());}

    

    // 4. 语音合成并播放void VoiceAssistant::handleListeningState() {

    if (synthesizeAndPlay(aiResponse)) {    unsigned long currentTime = millis();

        conversationCount++;    

        lastActivityTime = millis();    // 检查录音时间限制

        return true;    if (currentTime - recordingStartTime > MAX_RECORD_TIME) {

    } else {        Serial.println("录音时间超限，停止录音");

        handleError("语音合成失败");        stopListening();

        return false;        return;

    }    }

}    

    // 获取音频数据

bool VoiceAssistant::synthesizeAndPlay(const String& text) {    std::vector<int16_t> audioData = audioManager.getAudioData();

    Serial.printf("[VoiceAssistant] 合成语音: %s\n", text.c_str());    

        if (!audioData.empty()) {

    uint8_t* audioData = nullptr;        recordedAudio.insert(recordedAudio.end(), audioData.begin(), audioData.end());

    size_t audioSize = 0;        

            // 检测静默

    if (baiduSpeech->synthesizeSpeech(text, &audioData, &audioSize)) {        if (detectSilence(audioData)) {

        setState(VoiceState::SPEAKING);            if (silenceStartTime == 0) {

                        silenceStartTime = currentTime;

        // 播放音频数据            } else if (currentTime - silenceStartTime > SILENCE_TIMEOUT) {

        bool playSuccess = audioManager->playAudioData(audioData, audioSize);                // 静默超时，停止录音

                        if (currentTime - recordingStartTime > MIN_RECORD_TIME) {

        // 释放音频数据                    Serial.println("检测到静默，停止录音");

        free(audioData);                    stopListening();

                        } else {

        if (playSuccess) {                    Serial.println("录音时间过短，继续监听");

            Serial.println("[VoiceAssistant] 语音播放完成");                    resetRecording();

            return true;                    setState(STATE_IDLE);

        } else {                }

            Serial.println("[VoiceAssistant] 语音播放失败");            }

            return false;        } else {

        }            silenceStartTime = 0;

    } else {        }

        Serial.printf("[VoiceAssistant] 语音合成失败: %s\n", baiduSpeech->getLastError().c_str());    }

        return false;}

    }

}void VoiceAssistant::handleProcessingState() {

    // 处理状态在processAudio方法中处理

bool VoiceAssistant::detectWakeWord(const String& text) {    // 这里可以添加超时检查

    String lowerText = text;    unsigned long currentTime = millis();

    lowerText.toLowerCase();    if (currentTime - stateStartTime > 30000) { // 30秒超时

            Serial.println("处理超时，返回待机状态");

    for (int i = 0; i < 5; i++) {        setState(STATE_ERROR);

        String lowerWakeWord = wakeWords[i];    }

        lowerWakeWord.toLowerCase();}

        

        if (lowerText.indexOf(lowerWakeWord) >= 0) {void VoiceAssistant::handleSpeakingState() {

            return true;    // 播放状态在speakResponse方法中处理

        }    // 这里可以添加播放完成检查

    }}

    

    return false;void VoiceAssistant::handleErrorState() {

}    // 错误状态，等待一段时间后返回待机状态

    unsigned long currentTime = millis();

void VoiceAssistant::handleStateTimeout() {    if (currentTime - stateStartTime > 5000) { // 5秒后恢复

    unsigned long elapsed = millis() - stateStartTime;        Serial.println("从错误状态恢复");

            setState(STATE_IDLE);

    switch (currentState) {    }

        case VoiceState::RECORDING:}

            if (elapsed > recordTimeout) {

                Serial.println("[VoiceAssistant] 录音超时");bool VoiceAssistant::detectSilence(const std::vector<int16_t>& audioData) {

                setState(VoiceState::PROCESSING);    float rms = calculateRMS(audioData);

            }    return rms < silenceThreshold;

            break;}

            

        case VoiceState::PROCESSING:float VoiceAssistant::calculateRMS(const std::vector<int16_t>& audioData) {

            if (elapsed > processTimeout) {    if (audioData.empty()) {

                Serial.println("[VoiceAssistant] 处理超时");        return 0.0f;

                setState(VoiceState::ERROR);    }

            }    

            break;    float sum = 0.0f;

                for (int16_t sample : audioData) {

        default:        float normalized = sample / 32768.0f;

            break;        sum += normalized * normalized;

    }    }

}    

    return sqrt(sum / audioData.size());

void VoiceAssistant::handleError(const String& error) {}

    lastError = error;

    Serial.printf("[VoiceAssistant] 错误: %s\n", error.c_str());void VoiceAssistant::resetRecording() {

        recordedAudio.clear();

    // 播放错误提示音（简单的beep）    silenceStartTime = 0;

    // TODO: 可以合成语音错误提示    recordingStartTime = millis();

}}

String VoiceAssistant::processTextInput(const String& text) {
    if (!initialized) {
        return "语音助手未初始化";
    }
    
    Serial.printf("[VoiceAssistant] 处理文本输入: %s\n", text.c_str());
    
    String response = deepSeekClient->chat(text);
    conversationCount++;
    lastActivityTime = millis();
    
    return response;
}

VoiceState VoiceAssistant::getState() const {
    return currentState;
}

String VoiceAssistant::getStateString() const {
    return getStateString(currentState);
}

String VoiceAssistant::getStateString(VoiceState state) const {
    switch (state) {
        case VoiceState::IDLE: return "空闲";
        case VoiceState::LISTENING: return "监听";
        case VoiceState::RECORDING: return "录音";
        case VoiceState::PROCESSING: return "处理";
        case VoiceState::SPEAKING: return "播放";
        case VoiceState::ERROR: return "错误";
        default: return "未知";
    }
}

bool VoiceAssistant::isReady() const {
    return initialized && currentState != VoiceState::ERROR;
}

bool VoiceAssistant::startListening() {
    if (!initialized) {
        return false;
    }
    
    setState(VoiceState::LISTENING);
    return true;
}

bool VoiceAssistant::stopListening() {
    if (!initialized) {
        return false;
    }
    
    setState(VoiceState::IDLE);
    return true;
}

int VoiceAssistant::getConversationCount() const {
    return conversationCount;
}

unsigned long VoiceAssistant::getLastActivityTime() const {
    return lastActivityTime;
}