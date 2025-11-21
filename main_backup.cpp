#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include "config.h"
#include <BaiduSpeech.h>

// 函数声明
void setupLED();
void blinkLED();
void testLED();
void printDetailedStatus();
void printMemoryInfo();
void printWiFiInfo();
void printHelp();
void checkMemoryStatus();
void checkWiFiStatus();
void showHeartbeat();
void processSerialCommands();

// AI功能函数
void setupAI();
void printAIStatus();
String chatWithDeepSeek(const String& message);
void testAIServices();
void testNetworkConnectivity();

// 语音功能函数
void setupAudio();
void setupBaiduSpeech();
void testBaiduSpeech();
void startVoiceRecording();
void stopVoiceRecording();
void processVoiceInput();
void playTTSAudio(uint8_t* audioData, size_t audioSize);
void printSpeechStatus();

// 全局变量
unsigned long lastStatusReport = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastHeartbeat = 0;
bool wifiWasConnected = false;
int commandCount = 0;

// AI服务状态
bool aiInitialized = false;
int conversationCount = 0;

// 语音服务状态
BaiduSpeech baiduSpeech;
bool speechInitialized = false;
bool isRecording = false;
uint8_t* audioBuffer = nullptr;
size_t audioBufferSize = 0;
uint8_t* recordBuffer = nullptr;
size_t recordedSize = 0;

// 简化版本的LED管理
void setupLED() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void blinkLED() {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
}

void testLED() {
    Serial.println("[LED] Testing LED patterns...");
    
    // 快速闪烁
    Serial.println("[LED] Fast blink test...");
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    // 慢速闪烁
    Serial.println("[LED] Slow blink test...");
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
    }
    
    Serial.println("[LED] LED test completed");
}

void printDetailedStatus() {
    Serial.println("\n========================================");
    Serial.println("         DETAILED SYSTEM STATUS");
    Serial.println("========================================");
    
    // 硬件信息
    Serial.println("[HARDWARE]");
    Serial.printf("  Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("  Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("  CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("  Flash Speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    
    // 内存信息
    printMemoryInfo();
    
    // WiFi信息
    printWiFiInfo();
    
    // AI状态
    Serial.println("[AI SERVICES]");
    Serial.printf("  Status: %s\n", aiInitialized ? "✓ Ready" : "✗ Not initialized");
    Serial.printf("  Conversations: %d\n", conversationCount);
    
    // GPIO状态
    Serial.println("[GPIO]");
    Serial.printf("  LED Pin (GPIO%d): %s\n", LED_PIN, digitalRead(LED_PIN) ? "HIGH" : "LOW");
    
    Serial.println("========================================\n");
}

void printMemoryInfo() {
    Serial.println("[MEMORY]");
    Serial.printf("  Free Heap: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.printf("  Minimum Free Heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("  Heap Size: %d bytes\n", ESP.getHeapSize());
    Serial.printf("  Max Alloc Heap: %d bytes\n", ESP.getMaxAllocHeap());
    
    // PSRAM信息（如果有的话）
    if (ESP.getPsramSize() > 0) {
        Serial.printf("  PSRAM Size: %d bytes\n", ESP.getPsramSize());
        Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
    } else {
        Serial.println("  PSRAM: Not available");
    }
}

void printWiFiInfo() {
    Serial.println("[WIFI]");
    Serial.printf("  Status: %s\n", WiFi.status() == WL_CONNECTED ? "✓ Connected" : "✗ Disconnected");
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("  SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("  Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("  DNS: %s\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("  Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
        Serial.printf("  MAC Address: %s\n", WiFi.macAddress().c_str());
        Serial.printf("  Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("  Channel: %d\n", WiFi.channel());
        
        // 信号质量评估
        int rssi = WiFi.RSSI();
        String quality;
        if (rssi > -50) quality = "Excellent";
        else if (rssi > -60) quality = "Good";
        else if (rssi > -70) quality = "Fair";
        else quality = "Poor";
        Serial.printf("  Signal Quality: %s\n", quality.c_str());
    } else {
        Serial.printf("  Status Code: %d\n", WiFi.status());
        Serial.printf("  Configured SSID: %s\n", WIFI_SSID);
        Serial.println("  Recommendation: Check SSID and password in config.h");
    }
}

void checkWiFiStatus() {
    // 检查WiFi连接状态并在需要时重连
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiWasConnected) {
            Serial.println("[WIFI] 连接丢失，尝试重连...");
            wifiWasConnected = false;
        }
        
        // 尝试重连
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 5) {
            Serial.printf("[WIFI] 重连尝试 %d/5...", attempts + 1);
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            
            // 等待连接，最多20秒
            int waitTime = 0;
            while (WiFi.status() != WL_CONNECTED && waitTime < 20) {
                delay(1000);
                Serial.print(".");
                waitTime++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\n[WIFI] ✓ 重连成功！");
                Serial.printf("[WIFI] IP地址: %s\n", WiFi.localIP().toString().c_str());
                wifiWasConnected = true;
                break;
            } else {
                Serial.println("\n[WIFI] ✗ 重连失败");
                attempts++;
            }
        }
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WIFI] ✗ 无法重新连接WiFi");
        }
    } else {
        if (!wifiWasConnected) {
            Serial.println("[WIFI] ✓ WiFi连接正常");
            wifiWasConnected = true;
        }
    }
}

void testNetworkConnectivity() {
    Serial.println("\n=== 网络连接诊断 ===");
    
    // WiFi状态检查
    Serial.printf("WiFi状态: %s\n", WiFi.status() == WL_CONNECTED ? "已连接" : "未连接");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("IP地址: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("信号强度: %d dBm\n", WiFi.RSSI());
        Serial.printf("网关: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
    }
    
    // DNS解析测试
    Serial.println("\n测试DNS解析...");
    IPAddress resolvedIP;
    if (WiFi.hostByName("api.deepseek.com", resolvedIP)) {
        Serial.printf("✓ DNS解析成功: api.deepseek.com -> %s\n", resolvedIP.toString().c_str());
    } else {
        Serial.println("✗ DNS解析失败");
        return;
    }
    
    // TCP连接测试
    Serial.println("\n测试TCP连接...");
    WiFiClient testClient;
    testClient.setTimeout(10000);
    
    if (testClient.connect("api.deepseek.com", 443)) {
        Serial.println("✓ TCP连接成功 (端口 443)");
        testClient.stop();
    } else {
        Serial.println("✗ TCP连接失败");
    }
    
    // SSL连接测试
    Serial.println("\n测试SSL连接...");
    WiFiClientSecure testSecureClient;
    testSecureClient.setInsecure();
    testSecureClient.setTimeout(15000);
    
    if (testSecureClient.connect("api.deepseek.com", 443)) {
        Serial.println("✓ SSL连接成功");
        testSecureClient.stop();
    } else {
        Serial.println("✗ SSL连接失败");
    }
    
    Serial.println("===================");
}

void printHelp() {
    Serial.println("\n========================================");
    Serial.println("           AVAILABLE COMMANDS");
    Serial.println("========================================");
    Serial.println("系统命令:");
    Serial.println("  ping     - 测试串口响应");
    Serial.println("  status   - 显示详细系统状态");
    Serial.println("  wifi     - 显示WiFi信息");
    Serial.println("  memory   - 显示内存信息");
    Serial.println("  nettest  - 测试网络连接");
    Serial.println("  led      - 运行LED测试");
    Serial.println("  restart  - 重启系统");
    Serial.println("  reconnect - 强制重连WiFi");
    Serial.println("  cleanup  - 清理网络连接和内存");
    Serial.println("  help     - 显示帮助信息");
    Serial.println();
    Serial.println("AI功能:");
    Serial.println("  ai       - 显示AI服务状态");
    Serial.println("  test     - 测试AI服务连接");
    Serial.println("  chat [消息] - 与AI对话");
    Serial.println("  例如: chat 你好");
    Serial.println();
    Serial.println("语音功能:");
    Serial.println("  speech   - 显示语音服务状态");
    Serial.println("  speechtest - 测试语音服务");
    Serial.println("  record   - 开始语音录制");
    Serial.println("  stop     - 停止语音录制");
    Serial.println("  tts [文本] - 文本转语音");
    Serial.println();
    Serial.println("直接输入中文即可与小智对话！");
    Serial.println("========================================");
    Serial.println("提示: 命令不区分大小写");
    Serial.println("提示: 系统每30秒显示一次心跳");
    Serial.println("========================================\n");
}

String chatWithDeepSeek(const String& message) {
    if (WiFi.status() != WL_CONNECTED) {
        return "网络未连接，请检查WiFi设置";
    }
    
    // 检查内存是否足够 - 降低内存要求
    if (ESP.getFreeHeap() < 50000) {
        Serial.println("[DeepSeek] 内存不足，跳过此次请求");
        return "内存不足，请输入 'restart' 重启系统";
    }
    
    Serial.printf("[DeepSeek] 准备请求: %s\n", message.c_str());
    Serial.printf("[DeepSeek] 可用内存: %d bytes\n", ESP.getFreeHeap());
    
    // 使用HTTPClient而不是直接SSL连接
    HTTPClient http;
    WiFiClientSecure client;
    
    // 配置客户端
    client.setInsecure();
    client.setTimeout(15000); // 减少超时时间
    
    // 尝试使用HTTPClient
    http.begin(client, "https://api.deepseek.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(DEEPSEEK_API_KEY));
    http.setTimeout(15000);
    
    // 构建简化的请求体
    String requestBody = "{";
    requestBody += "\"model\":\"deepseek-chat\",";
    requestBody += "\"messages\":[{\"role\":\"user\",\"content\":\"" + message + "\"}],";
    requestBody += "\"max_tokens\":30,"; // 减少token数量
    requestBody += "\"stream\":false}";
    
    Serial.printf("[DeepSeek] 发送HTTP请求...\n");
    
    int httpResponseCode = http.POST(requestBody);
    
    String response = "";
    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println("[DeepSeek] ✓ 请求成功");
        
        // 解析JSON响应
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            if (doc.containsKey("choices") && doc["choices"].size() > 0) {
                response = doc["choices"][0]["message"]["content"].as<String>();
                conversationCount++;
            } else {
                response = "AI响应格式异常";
            }
        } else {
            response = "JSON解析失败";
        }
    } else {
        Serial.printf("[DeepSeek] ✗ HTTP错误: %d\n", httpResponseCode);
        response = "服务器连接失败 (错误码: " + String(httpResponseCode) + ")";
    }
    
    // 清理资源
    http.end();
    client.stop();
    
    return response;
}

void setupAI() {
    Serial.println("[AI] 开始初始化AI服务...");
    
    // 等待WiFi稳定
    delay(2000);
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[AI] WiFi未连接，跳过AI初始化");
        return;
    }
    
    // 测试DeepSeek连接
    Serial.println("[AI] 测试DeepSeek API连接...");
    String testResponse = chatWithDeepSeek("你好");
    
    if (testResponse.indexOf("网络") == -1 && testResponse.indexOf("失败") == -1 && 
        testResponse.indexOf("超时") == -1 && testResponse.indexOf("不可用") == -1) {
        aiInitialized = true;
        Serial.println("[AI] ✓ AI服务初始化成功！");
        Serial.printf("[AI] 测试回复: %s\n", testResponse.c_str());
        
        // 成功指示
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(200);
            digitalWrite(LED_PIN, LOW);
            delay(200);
        }
    } else {
        Serial.println("[AI] ✗ AI服务初始化失败");
        Serial.printf("[AI] 错误信息: %s\n", testResponse.c_str());
        
        // 错误指示
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    }
}

void printAIStatus() {
    Serial.println("=== AI服务状态 ===");
    Serial.printf("DeepSeek API: %s\n", aiInitialized ? "✓ 已连接" : "✗ 未连接");
    Serial.printf("API Key: %s\n", String(DEEPSEEK_API_KEY).substring(0, 10) + "...");
    Serial.printf("对话次数: %d次\n", conversationCount);
    Serial.printf("网络状态: %s\n", WiFi.status() == WL_CONNECTED ? "✓ 已连接" : "✗ 未连接");
    
    // 内存状态
    size_t freeHeap = ESP.getFreeHeap();
    Serial.printf("可用内存: %d bytes\n", freeHeap);
    
    if (freeHeap < 100000) {
        Serial.println("⚠️ 警告: 内存不足，可能影响AI功能");
    }
    
    Serial.println();
}

// 语音功能实现
void setupAudio() {
    Serial.println("[AUDIO] 初始化I2S音频接口...");
    
    // 配置I2S音频输入 (麦克风)
    i2s_config_t i2s_config_in = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config_in = {
        .bck_io_num = I2S_MIC_SCK_PIN,
        .ws_io_num = I2S_MIC_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD_PIN
    };
    
    // 安装I2S驱动
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config_in, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] I2S驱动安装失败: %d\n", err);
        return;
    }
    
    err = i2s_set_pin(I2S_NUM_0, &pin_config_in);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] I2S引脚配置失败: %d\n", err);
        return;
    }
    
    Serial.println("[AUDIO] ✓ I2S音频接口初始化成功");
    
    // 分配音频缓冲区
    audioBufferSize = SAMPLE_RATE * 2 * 5; // 5秒的16位音频
    audioBuffer = (uint8_t*)malloc(audioBufferSize);
    if (!audioBuffer) {
        Serial.println("[AUDIO] ✗ 音频缓冲区分配失败");
        return;
    }
    
    Serial.printf("[AUDIO] 音频缓冲区大小: %d bytes\n", audioBufferSize);
}

void setupBaiduSpeech() {
    Serial.println("[SPEECH] 初始化百度语音服务...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SPEECH] WiFi未连接，跳过语音服务初始化");
        return;
    }
    
    // 初始化百度语音服务
    if (baiduSpeech.begin(BAIDU_APP_ID, BAIDU_API_KEY, BAIDU_SECRET_KEY)) {
        speechInitialized = true;
        Serial.println("[SPEECH] ✓ 百度语音服务初始化成功");
        
        // 配置语音识别和合成参数
        baiduSpeech.setASRConfig("zh", SAMPLE_RATE);
        baiduSpeech.setTTSConfig("zh", 5, 5, 7);
        
        // 成功指示
        for (int i = 0; i < 2; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(150);
            digitalWrite(LED_PIN, LOW);
            delay(150);
        }
    } else {
        Serial.println("[SPEECH] ✗ 百度语音服务初始化失败");
        Serial.printf("[SPEECH] 错误: %s\n", baiduSpeech.getLastError().c_str());
        
        // 错误指示
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(80);
            digitalWrite(LED_PIN, LOW);
            delay(80);
        }
    }
}

void printSpeechStatus() {
    Serial.println("=== 语音服务状态 ===");
    Serial.printf("百度语音API: %s\n", speechInitialized ? "✓ 已连接" : "✗ 未连接");
    Serial.printf("App ID: %s\n", BAIDU_APP_ID);
    Serial.printf("录音状态: %s\n", isRecording ? "录音中" : "空闲");
    Serial.printf("音频配置: %dHz, 16bit, 单声道\n", SAMPLE_RATE);
    
    if (!speechInitialized && baiduSpeech.getLastError().length() > 0) {
        Serial.printf("最后错误: %s\n", baiduSpeech.getLastError().c_str());
    }
    
    Serial.println();
}

void testBaiduSpeech() {
    Serial.println("[TEST] 测试百度语音服务...");
    
    if (!speechInitialized) {
        Serial.println("[TEST] 语音服务未初始化，尝试重新初始化...");
        setupBaiduSpeech();
        return;
    }
    
    // 测试文本转语音
    Serial.println("[TEST] 测试文本转语音...");
    uint8_t* ttsAudio = nullptr;
    size_t ttsSize = 0;
    
    if (baiduSpeech.synthesizeSpeech("你好，我是小智语音助手", &ttsAudio, &ttsSize)) {
        Serial.printf("[TEST] ✓ TTS测试成功，音频大小: %d bytes\n", ttsSize);
        
        // 可以在这里播放音频
        if (ttsAudio) {
            free(ttsAudio);
        }
    } else {
        Serial.printf("[TEST] ✗ TTS测试失败: %s\n", baiduSpeech.getLastError().c_str());
    }
    
    Serial.println("[TEST] 语音服务测试完成");
}

void startVoiceRecording() {
    if (!speechInitialized) {
        Serial.println("[RECORD] 语音服务未初始化");
        return;
    }
    
    if (isRecording) {
        Serial.println("[RECORD] 已在录音中");
        return;
    }
    
    Serial.println("[RECORD] 开始语音录制...");
    isRecording = true;
    recordedSize = 0;
    
    // 清空I2S缓冲区
    i2s_zero_dma_buffer(I2S_NUM_0);
    
    // LED指示录音状态
    digitalWrite(LED_PIN, HIGH);
    
    Serial.println("[RECORD] ✓ 录音开始，请说话...");
    Serial.println("[RECORD] 输入 'stop' 停止录音");
}

void stopVoiceRecording() {
    if (!isRecording) {
        Serial.println("[RECORD] 当前未在录音");
        return;
    }
    
    isRecording = false;
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("[RECORD] 录音停止");
    Serial.printf("[RECORD] 录制音频大小: %d bytes\n", recordedSize);
    
    if (recordedSize > 0) {
        processVoiceInput();
    }
}

void processVoiceInput() {
    if (!speechInitialized || recordedSize == 0) {
        Serial.println("[PROCESS] 无法处理语音输入");
        return;
    }
    
    Serial.println("[PROCESS] 开始语音识别...");
    
    // 语音识别
    String recognizedText = baiduSpeech.recognizeSpeech(recordBuffer, recordedSize, "pcm");
    
    if (recognizedText.length() > 0) {
        Serial.printf("[PROCESS] 识别结果: %s\n", recognizedText.c_str());
        
        // 与AI对话
        Serial.println("[PROCESS] 发送给AI...");
        String aiResponse = chatWithDeepSeek(recognizedText);
        
        Serial.printf("[PROCESS] AI回复: %s\n", aiResponse.c_str());
        
        // 文本转语音
        Serial.println("[PROCESS] 生成语音回复...");
        uint8_t* ttsAudio = nullptr;
        size_t ttsSize = 0;
        
        if (baiduSpeech.synthesizeSpeech(aiResponse, &ttsAudio, &ttsSize)) {
            Serial.printf("[PROCESS] 语音合成成功，播放回复...\n");
            playTTSAudio(ttsAudio, ttsSize);
            
            if (ttsAudio) {
                free(ttsAudio);
            }
        } else {
            Serial.printf("[PROCESS] 语音合成失败: %s\n", baiduSpeech.getLastError().c_str());
        }
    } else {
        Serial.printf("[PROCESS] 语音识别失败: %s\n", baiduSpeech.getLastError().c_str());
    }
    
    // 清空录音缓冲区
    recordedSize = 0;
}

void playTTSAudio(uint8_t* audioData, size_t audioSize) {
    Serial.printf("[AUDIO] 播放音频: %d bytes\n", audioSize);
    
    // 这里可以通过I2S输出音频到功放
    // 由于功放配置较复杂，暂时用LED闪烁表示播放
    for (int i = 0; i < 10; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
        delay(50);
    }
    
    Serial.println("[AUDIO] 音频播放完成");
}

void testAIServices() {
    Serial.println("[TEST] 开始AI服务测试...");
    
    if (!aiInitialized) {
        Serial.println("[TEST] AI服务未初始化，尝试重新初始化...");
        setupAI();
        return;
    }
    
    // 测试多个问题
    String testQuestions[] = {
        "你好",
        "你是谁？",
        "今天天气怎么样？"
    };
    
    for (int i = 0; i < 3; i++) {
        Serial.printf("[TEST] 测试问题 %d: %s\n", i+1, testQuestions[i].c_str());
        String response = chatWithDeepSeek(testQuestions[i]);
        Serial.printf("[TEST] AI回复 %d: %s\n", i+1, response.c_str());
        delay(1000); // 避免请求过快
    }
    
    Serial.println("[TEST] AI服务测试完成！");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("=== ESP32-S3 Voice Assistant XiaoZhi ===");
    Serial.println("========================================");
    Serial.println("Firmware Version: v1.2.0 (AI Integrated)");
    Serial.println("Build Date: " __DATE__ " " __TIME__);
    Serial.printf("ESP32 Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("ESP32 Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.println("========================================");
    
    Serial.println("[INIT] Starting hardware initialization...");
    
    // Initialize LED
    Serial.println("[LED] Initializing LED control...");
    setupLED();
    Serial.printf("[LED] LED Pin: GPIO%d\n", LED_PIN);
    Serial.println("[LED] LED initialization complete");
    
    // LED startup indication
    Serial.println("[LED] Running startup LED test...");
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
    
    // Connect WiFi
    Serial.println("[WIFI] Starting WiFi connection...");
    Serial.printf("[WIFI] SSID: %s\n", WIFI_SSID);
    Serial.println("[WIFI] Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    unsigned long wifiStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        blinkLED();
        attempts++;
        
        // Show progress every 5 attempts
        if (attempts % 5 == 0) {
            Serial.printf("\n[WIFI] Connection attempt %d/30...", attempts);
        }
    }
    
    unsigned long wifiConnectTime = millis() - wifiStartTime;
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] ✓ WiFi connected successfully!");
        Serial.printf("[WIFI] IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WIFI] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("[WIFI] DNS: %s\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("[WIFI] Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("[WIFI] Connection time: %lu ms\n", wifiConnectTime);
        Serial.printf("[WIFI] MAC Address: %s\n", WiFi.macAddress().c_str());
        
        // Success indication
        Serial.println("[LED] WiFi success indication...");
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(200);
            digitalWrite(LED_PIN, LOW);
            delay(200);
        }
        
        // 初始化AI服务
        Serial.println("[AI] 开始AI服务初始化...");
        setupAI();
        
        // 初始化音频系统
        Serial.println("[AUDIO] 开始音频系统初始化...");
        setupAudio();
        
        // 初始化语音服务
        Serial.println("[SPEECH] 开始语音服务初始化...");
        setupBaiduSpeech();
        
    } else {
        Serial.println("\n[WIFI] ✗ WiFi connection failed!");
        Serial.printf("[WIFI] Status code: %d\n", WiFi.status());
        Serial.println("[WIFI] Please check SSID and password in config.h");
        
        // Error indication
        Serial.println("[LED] WiFi error indication...");
        for (int i = 0; i < 10; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    }
    
    Serial.println("\n========================================");
    Serial.println("[INIT] System initialization completed!");
    Serial.println("[SYSTEM] Device is ready for operation");
    if (aiInitialized) {
        Serial.println("[AI] 🤖 AI服务已就绪！现在可以与小智对话了");
        Serial.println("[AI] 💬 直接输入中文即可开始对话");
    }
    Serial.println("[HELP] Type 'help' to see available commands");
    Serial.println("[TEST] Type 'ping' to test serial communication");
    Serial.println("========================================");
    Serial.println("[SYSTEM] Entering main loop...");
    Serial.println("[SERIAL] Serial port ready, waiting for commands...\n");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Heartbeat每30秒显示一次，并进行系统维护
    if (currentTime - lastHeartbeat > 30000) {
        lastHeartbeat = currentTime;
        Serial.printf("[HEARTBEAT] System running - Uptime: %lu seconds", currentTime / 1000);
        if (aiInitialized) {
            Serial.printf(" | AI: Ready | Conversations: %d", conversationCount);
        }
        Serial.println();
        
        // 显示内存状态
        uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 100000) {  // 如果可用内存少于100KB，警告
            Serial.printf("[WARNING] Low memory: %d bytes\n", freeHeap);
        }
        
        // 定期清理，防止内存泄漏
        if (conversationCount > 0 && (conversationCount % 5 == 0)) {
            Serial.println("[MAINTENANCE] 定期清理内存...");
            ESP.getMaxAllocHeap(); // 触发垃圾回收
        }
        
        // 内存过低时的紧急清理
        if (freeHeap < 60000) {
            Serial.println("[MAINTENANCE] 内存不足，执行紧急清理...");
            ESP.getMaxAllocHeap();
            
            if (ESP.getFreeHeap() < 50000) {
                Serial.println("[CRITICAL] 内存严重不足，建议重启系统");
                Serial.println("[HELP] 输入 'restart' 重启系统");
            }
        }
    }
    
    // WiFi状态检查和自动重连
    if (currentTime - lastWiFiCheck > 15000) {  // 每15秒检查一次WiFi
        lastWiFiCheck = currentTime;
        
        bool wasConnected = wifiWasConnected;
        checkWiFiStatus();  // 使用我们的新函数
        
        bool isConnected = WiFi.status() == WL_CONNECTED;
        
        if (isConnected && !wasConnected) {
            Serial.println("[WIFI] ✓ WiFi connection restored!");
            Serial.printf("[WIFI] IP: %s, Signal: %d dBm\n", 
                         WiFi.localIP().toString().c_str(), WiFi.RSSI());
            
            // 恢复连接指示
            for (int i = 0; i < 3; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
            }
            
            // 重新初始化AI服务
            if (!aiInitialized) {
                setupAI();
            }
            
        } else if (!isConnected && wasConnected) {
            Serial.println("[WIFI] ✗ WiFi connection lost!");
            aiInitialized = false; // WiFi断开时禁用AI
            
            // 连接丢失指示
            for (int i = 0; i < 5; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(50);
                digitalWrite(LED_PIN, LOW);
                delay(50);
            }
        }
    }
    
    // 处理串口命令
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        // 添加调试输出
        Serial.printf("[DEBUG] Raw input: '%s' (length: %d)\n", input.c_str(), input.length());
        
        if (input.length() > 0) {
            commandCount++;
            Serial.printf("[CMD] Processing #%d: '%s'\n", commandCount, input.c_str());
            
            // 简单的命令反馈测试
            Serial.println("[INFO] Command received, processing...");
            
            // 转换为小写进行命令识别
            String command = input;
            command.toLowerCase();
            
            // 添加基本命令测试
            if (command == "ping") {
                Serial.println("[PING] Pong! System is responsive.");
                return;
            }
            
            if (command == "status") {
                Serial.println("[STATUS] System status report:");
                printDetailedStatus();
            } else if (command == "restart") {
                Serial.println("[SYSTEM] ⚠️ System restarting in 3 seconds...");
                for (int i = 3; i > 0; i--) {
                    Serial.printf("[SYSTEM] Restart countdown: %d\n", i);
                    digitalWrite(LED_PIN, HIGH);
                    delay(500);
                    digitalWrite(LED_PIN, LOW);
                    delay(500);
                }
                ESP.restart();
            } else if (command == "led") {
                Serial.println("[LED] Running LED test sequence...");
                testLED();
            } else if (command == "wifi") {
                Serial.println("[WIFI] WiFi information:");
                printWiFiInfo();
            } else if (command == "memory") {
                Serial.println("[MEMORY] Memory information:");
                printMemoryInfo();
            } else if (command == "nettest") {
                Serial.println("[NETWORK] Testing network connectivity...");
                testNetworkConnectivity();
            } else if (command == "reconnect") {
                Serial.println("[WIFI] 强制重新连接WiFi...");
                WiFi.disconnect();
                delay(1000);
                checkWiFiStatus();
            } else if (command == "reset") {
                Serial.println("[SYSTEM] 软重启系统...");
                ESP.restart();
            } else if (command == "cleanup") {
                Serial.println("[MAINTENANCE] 手动清理内存...");
                uint32_t beforeHeap = ESP.getFreeHeap();
                ESP.getMaxAllocHeap(); // 触发垃圾回收
                uint32_t afterHeap = ESP.getFreeHeap();
                Serial.printf("[CLEANUP] 内存清理完成: %d -> %d bytes (+%d)\n", 
                             beforeHeap, afterHeap, afterHeap - beforeHeap);
            } else if (command == "help") {
                printHelp();
            } else if (command == "ai") {
                Serial.println("[AI] AI services status:");
                printAIStatus();
            } else if (command == "test") {
                testAIServices();
            } else if (command == "speech") {
                Serial.println("[SPEECH] Speech services status:");
                printSpeechStatus();
            } else if (command == "speechtest") {
                testBaiduSpeech();
            } else if (command == "record") {
                startVoiceRecording();
            } else if (command == "stop") {
                if (isRecording) {
                    stopVoiceRecording();
                } else {
                    Serial.println("当前未在录音");
                }
            } else if (command.startsWith("tts ")) {
                // 文本转语音测试
                String text = input.substring(4);
                if (speechInitialized) {
                    Serial.printf("TTS: %s\n", text.c_str());
                    uint8_t* ttsAudio = nullptr;
                    size_t ttsSize = 0;
                    
                    if (baiduSpeech.synthesizeSpeech(text, &ttsAudio, &ttsSize)) {
                        Serial.printf("语音合成成功，播放音频...\n");
                        playTTSAudio(ttsAudio, ttsSize);
                        if (ttsAudio) {
                            free(ttsAudio);
                        }
                    } else {
                        Serial.printf("语音合成失败: %s\n", baiduSpeech.getLastError().c_str());
                    }
                } else {
                    Serial.println("语音服务未初始化");
                }
            } else if (command.startsWith("chat ")) {
                // 提取聊天消息
                String message = input.substring(5);
                if (aiInitialized) {
                    Serial.printf("你: %s\n", message.c_str());
                    String response = chatWithDeepSeek(message);
                    Serial.printf("小智: %s\n", response.c_str());
                } else {
                    Serial.println("AI服务未初始化，请检查网络连接");
                }
            } else {
                // 尝试作为聊天消息处理
                if (aiInitialized && input.length() > 0) {
                    Serial.printf("你: %s\n", input.c_str());
                    String response = chatWithDeepSeek(input);
                    Serial.printf("小智: %s\n", response.c_str());
                } else if (!aiInitialized) {
                    Serial.println("AI服务未初始化，请检查网络连接或输入'ai'查看状态");
                } else {
                    Serial.printf("[CMD] ✗ Unknown command: '%s'\n", command.c_str());
                    Serial.println("[HELP] Type 'help' to see available commands");
                }
            }
        }
    }
    
    // 处理语音录制
    if (isRecording && audioBuffer && recordedSize < audioBufferSize - 1024) {
        size_t bytes_read = 0;
        uint8_t* temp_buffer = audioBuffer + recordedSize;
        
        esp_err_t result = i2s_read(I2S_NUM_0, temp_buffer, 1024, &bytes_read, 0);
        if (result == ESP_OK && bytes_read > 0) {
            recordedSize += bytes_read;
            recordBuffer = audioBuffer; // 指向录音数据
            
            // 显示录音进度
            if (recordedSize % 8192 == 0) {
                Serial.printf("[RECORD] 已录制: %d bytes\n", recordedSize);
            }
        }
    }
    
    // Keep WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
    }
    
    delay(100);
}