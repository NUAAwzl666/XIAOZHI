#include "LEDManager.h"
#include "config.h"

LEDManager ledManager;

LEDManager::LEDManager() : currentState(LED_OFF), lastUpdate(0), breathPhase(0), blinkState(false), 
                          currentBrightness(0), maxBrightnessPercent(LED_MAX_BRIGHTNESS_PERCENT) {
}

bool LEDManager::initialize() {
    Serial.println("初始化简单红色LED...");
    
    // 配置GPIO引脚为输出模式
    pinMode(LED_PIN, OUTPUT);
    
    // 初始化LED为关闭状态
    digitalWrite(LED_PIN, LOW);
    currentBrightness = 0;
    
    Serial.println("简单红色LED初始化成功");
    return true;
}

void LEDManager::setState(LEDState state) {
    if (currentState != state) {
        currentState = state;
        lastUpdate = millis();
        breathPhase = 0;
        blinkState = false;
        
        switch (state) {
            case LED_OFF:
                turnOff();
                break;
            case LED_LISTENING:
                setBrightness(100); // 软件限制后约30亮度
                break;
            case LED_PROCESSING:
                setBrightness(200); // 软件限制后约60亮度
                break;
            case LED_SPEAKING:
                setBrightness(150); // 软件限制后约45亮度
                break;
            case LED_ERROR:
                setBrightness(255); // 软件限制后约77亮度
                break;
            case LED_CONNECTING:
                setBrightness(120); // 软件限制后约36亮度
                break;
        }
    }
}

void LEDManager::update() {
    unsigned long currentTime = millis();
    
    switch (currentState) {
        case LED_OFF:
            // 不需要更新
            break;
        case LED_LISTENING:
            updateListening();
            break;
        case LED_PROCESSING:
            updateProcessing();
            break;
        case LED_SPEAKING:
            updateSpeaking();
            break;
        case LED_ERROR:
            updateError();
            break;
        case LED_CONNECTING:
            updateConnecting();
            break;
    }
    
    lastUpdate = currentTime;
}

void LEDManager::setBrightness(uint8_t brightness) {
    // 软件限流：将最大亮度限制到安全范围以保护LED
    uint8_t safeBrightness = (brightness * maxBrightnessPercent) / 100;
    currentBrightness = safeBrightness;
    analogWrite(LED_PIN, safeBrightness);
}

void LEDManager::turnOn() {
    // 软件限流：使用安全亮度代替最大亮度
    currentBrightness = (255 * maxBrightnessPercent) / 100;
    analogWrite(LED_PIN, currentBrightness);
}

void LEDManager::turnOff() {
    currentBrightness = 0;
    digitalWrite(LED_PIN, LOW);
}

void LEDManager::setMaxBrightnessPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    maxBrightnessPercent = percent;
    Serial.printf("LED最大亮度设置为: %d%%\n", percent);
}

uint8_t LEDManager::getMaxBrightnessPercent() {
    return maxBrightnessPercent;
}

void LEDManager::breathe() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate > 50) {
        breathPhase++;
        // 计算呼吸亮度，然后应用软件限流
        uint8_t rawBrightness = (sin8(breathPhase * 2) / 2) + 127;
        uint8_t safeBrightness = (rawBrightness * maxBrightnessPercent) / 100;
        analogWrite(LED_PIN, safeBrightness);
    }
}

void LEDManager::blink(uint16_t interval) {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate > interval) {
        blinkState = !blinkState;
        
        if (blinkState) {
            analogWrite(LED_PIN, currentBrightness);
        } else {
            digitalWrite(LED_PIN, LOW);
        }
    }
}

void LEDManager::pulse() {
    breathe();
}

void LEDManager::fastBlink() {
    blink(200);
}

void LEDManager::slowBlink() {
    blink(1000);
}

void LEDManager::updateListening() {
    breathe(); // 呼吸效果表示监听
}

void LEDManager::updateProcessing() {
    fastBlink(); // 快速闪烁表示处理
}

void LEDManager::updateSpeaking() {
    pulse(); // 脉冲效果表示说话
}

void LEDManager::updateError() {
    blink(300); // 中速闪烁表示错误
}

void LEDManager::updateConnecting() {
    slowBlink(); // 慢速闪烁表示连接
}