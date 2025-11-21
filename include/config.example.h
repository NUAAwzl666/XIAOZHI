#ifndef CONFIG_H
#define CONFIG_H

// WiFi配置 - 请修改为您的WiFi信息
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// 硬件引脚配置
// I2S音频输入 (441麦克风)
#define I2S_MIC_WS_PIN    GPIO_NUM_45
#define I2S_MIC_SCK_PIN   GPIO_NUM_21
#define I2S_MIC_SD_PIN    GPIO_NUM_47

// I2S音频输出 (98357功放)
#define I2S_SPK_WS_PIN    GPIO_NUM_42
#define I2S_SPK_SCK_PIN   GPIO_NUM_41
#define I2S_SPK_SD_PIN    GPIO_NUM_40

// LED灯配置 (简单红色LED二极管)
#define LED_PIN           GPIO_NUM_48
#define LED_COUNT         1

// 音频配置
#define SAMPLE_RATE       16000
#define BITS_PER_SAMPLE   16
#define CHANNELS          1
#define BUFFER_SIZE       1024

// AI服务配置 - 请填入您的API密钥
#define DEEPSEEK_API_KEY  "sk-your-deepseek-api-key-here"
#define DEEPSEEK_BASE_URL "https://api.deepseek.com"

#define BAIDU_APP_ID      "your-baidu-app-id"
#define BAIDU_API_KEY     "your-baidu-api-key"
#define BAIDU_SECRET_KEY  "your-baidu-secret-key"

// 语音识别配置
#define WAKE_WORD         "小智"
#define SILENCE_TIMEOUT   3000  // 静默超时时间(ms)
#define MIN_RECORD_TIME   1000  // 最小录音时间(ms)
#define MAX_RECORD_TIME   10000 // 最大录音时间(ms)

#endif