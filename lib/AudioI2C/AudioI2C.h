#pragma once

#include <Arduino.h>

namespace AudioI2C {

// 初始化音频控制（不改I2S配置，仅初始化库状态）
bool begin();

// 设置音量 0..100
void setVolume(int vol);

// 增减音量
void increase();
void decrease();

// 获取当前音量
int getVolume();

// 获取软件增益（线性倍数，供PCM缩放使用）
float getSoftwareGain();

}
