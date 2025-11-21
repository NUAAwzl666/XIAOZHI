#ifndef CONFIG_H
#define CONFIG_H

// WiFi配置
#define WIFI_SSID "1234"
#define WIFI_PASSWORD "123456sss"

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

// 按钮配置
#define BUTTON_PIN        GPIO_NUM_1    // 使用GPIO1引脚替代GPIO0
#define BUTTON_DEBOUNCE   50            // 按钮防抖延时(ms)

// 音频配置
#define SAMPLE_RATE       16000
#define BITS_PER_SAMPLE   16
#define CHANNELS          1
#define BUFFER_SIZE       1024

// AI服务配置
#define DEEPSEEK_API_KEY  "sk-5f4807087b154707b6586f6eae1fc8c0"
#define DEEPSEEK_BASE_URL "https://api.deepseek.com"

#define BAIDU_APP_ID      "120197558"
#define BAIDU_API_KEY     "iw0uVTjdNE3EHj5I0ZliSj8Z"
#define BAIDU_SECRET_KEY  "M34aLHQUH2mkK3MDLQ540jfpiVUJpz6n"

// 语音识别配置
#define WAKE_WORD         "小智"
#define SILENCE_TIMEOUT   3000  // 静默超时时间(ms)
#define MIN_RECORD_TIME   500   // 最小录音时间(ms) - 降低到0.5秒
#define MAX_RECORD_TIME   2000  // 最大录音时间(ms) - 降低到2秒以节省内存（RAW格式优化）

#endif