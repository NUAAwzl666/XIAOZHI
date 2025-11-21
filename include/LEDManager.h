#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

// 软件限流配置：限制LED最大亮度百分比
#define LED_MAX_BRIGHTNESS_PERCENT 30

enum LEDState {
    LED_OFF,
    LED_LISTENING,
    LED_PROCESSING,
    LED_SPEAKING,
    LED_ERROR,
    LED_CONNECTING
};

class LEDManager {
private:
    LEDState currentState;
    unsigned long lastUpdate;
    uint8_t breathPhase;
    bool blinkState;
    uint8_t currentBrightness;
    uint8_t maxBrightnessPercent;
    
public:
    LEDManager();
    
    // 初始化方法
    bool initialize();
    
    // 状态控制
    void setState(LEDState state);
    void update();
    
    // 直接亮度控制
    void setBrightness(uint8_t brightness);
    void turnOn();
    void turnOff();
    
    // 安全设置 (用于无电阻连接)
    void setMaxBrightnessPercent(uint8_t percent);
    uint8_t getMaxBrightnessPercent();
    
    // 效果方法
    void breathe();
    void blink(uint16_t interval);
    void pulse();
    void fastBlink();
    void slowBlink();
    
private:
    void updateListening();
    void updateProcessing();
    void updateSpeaking();
    void updateError();
    void updateConnecting();
};

extern LEDManager ledManager;

#endif