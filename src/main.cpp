#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include "config.h"
#include <BaiduSpeech.h>
#include <BaiduRealtimeASR.h>

// 函数声明
void setupLED();
void blinkLED();
void testLED();
void setupButton();
void checkButton();
void handleButtonPress();
void handleButtonRelease();
void printDetailedStatus();
void printMemoryInfo();
void printWiFiInfo();
void printHelp();
void checkMemoryStatus();
void checkWiFiStatus();
void showHeartbeat();
void processSerialCommands();
void initializeSpeakerAndSilence();

// AI功能函数
void setupAI();
void printAIStatus();
String chatWithDeepSeek(const String& message);
void testAIServices();
void testNetworkConnectivity();

// 语音功能函数
void setupAudio();
void setupBaiduSpeech();
void setupRealtimeASR();
void testBaiduSpeech();
void startVoiceRecording();
void stopVoiceRecording();
void processVoiceInput();
void startRealtimeRecording();
void stopRealtimeRecording();
void startVoiceConversation();
void playTTSAudio(uint8_t* audioData, size_t audioSize);
bool speakTextStream(const String& text);
static bool ensureSpeakerI2S();
static bool writePCMToI2S(const uint8_t* data, size_t len);
void playTestTone(int frequency, int durationMs);
void printSpeechStatus();

// 实时识别回调函数
void onRealtimePartialResult(const String& result);
void onRealtimeFinalResult(const String& result, uint32_t startTime, uint32_t endTime);
void onRealtimeError(int errNo, const String& errMsg);
void onRealtimeConnected();
void onRealtimeDisconnected();

// 全局变量
unsigned long lastStatusReport = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastHeartbeat = 0;
bool wifiWasConnected = false;
int commandCount = 0;

// 按钮状态变量
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
bool isRecording = false;
unsigned long recordingStartTime = 0;  // 添加录音开始时间追踪

// AI服务状态
bool aiInitialized = false;
int conversationCount = 0;

// 语音服务状态
BaiduSpeech baiduSpeech;
BaiduRealtimeASR realtimeASR;
bool speechInitialized = false;
bool realtimeASRInitialized = false;
bool useRealtimeASR = true;  // 默认使用实时识别
uint8_t* audioBuffer = nullptr;
size_t audioBufferSize = 0;
uint8_t* recordBuffer = nullptr;
size_t recordedSize = 0;
String fullRecognizedText = "";  // 存储完整识别结果

// 系统初始化状态
bool systemFullyInitialized = false;  // 标记系统是否完全初始化完成

// 扬声器输出状态
static bool speakerInitialized = false;

// LED状态管理
enum LEDMode {
    LED_OFF,           // 灭
    LED_ON,            // 常亮
    LED_BLINK_FAST,    // 快闪（连接中）
    LED_BLINK_SLOW     // 慢闪（错误）
};

LEDMode currentLEDMode = LED_OFF;
unsigned long lastLEDToggle = 0;
bool ledState = false;

// LED控制函数
void setLEDMode(LEDMode mode) {
    currentLEDMode = mode;
    lastLEDToggle = millis();
    
    // 立即应用新状态
    if (mode == LED_OFF) {
        digitalWrite(LED_PIN, LOW);
        ledState = false;
    } else if (mode == LED_ON) {
        digitalWrite(LED_PIN, HIGH);
        ledState = true;
    }
}

void updateLED() {
    unsigned long now = millis();
    
    switch (currentLEDMode) {
        case LED_OFF:
            if (ledState) {
                digitalWrite(LED_PIN, LOW);
                ledState = false;
            }
            break;
            
        case LED_ON:
            if (!ledState) {
                digitalWrite(LED_PIN, HIGH);
                ledState = true;
            }
            break;
            
        case LED_BLINK_FAST:
            // 100ms间隔快闪
            if (now - lastLEDToggle > 100) {
                ledState = !ledState;
                digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                lastLEDToggle = now;
            }
            break;
            
        case LED_BLINK_SLOW:
            // 500ms间隔慢闪
            if (now - lastLEDToggle > 500) {
                ledState = !ledState;
                digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                lastLEDToggle = now;
            }
            break;
    }
}

// 简化版本的LED管理
void setupLED() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    setLEDMode(LED_OFF);
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

// 按钮管理功能
void setupButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.printf("[BUTTON] 外部按钮已初始化在GPIO%d\n", BUTTON_PIN);
    Serial.println("[BUTTON] 硬件连接方式:");
    Serial.printf("[BUTTON]   GPIO%d ---- 按钮 ---- GND\n", BUTTON_PIN);
    Serial.println("[BUTTON]   (使用内部上拉电阻，无需外部电阻)");
    Serial.println("[BUTTON] 按下按钮开始语音对话");
    
    // 初始化按钮状态
    lastButtonState = digitalRead(BUTTON_PIN);
    currentButtonState = lastButtonState;
    
    Serial.printf("[BUTTON] 初始状态: %s\n", lastButtonState ? "HIGH (未按下)" : "LOW (按下)");
}

void checkButton() {
    // 读取当前按钮状态
    bool reading = digitalRead(BUTTON_PIN);
    
    // 检查按钮状态是否改变（防抖处理）
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
        Serial.printf("[BUTTON] 状态变化: %s -> %s\n", 
                     lastButtonState ? "HIGH" : "LOW", 
                     reading ? "HIGH" : "LOW");
    }
    
    // 如果状态稳定超过防抖时间
    if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE) {
        // 如果按钮状态真的改变了
        if (reading != currentButtonState) {
            currentButtonState = reading;
            Serial.printf("[BUTTON] 确认状态: %s\n", currentButtonState ? "HIGH (释放)" : "LOW (按下)");
            
            // 按钮被按下（从HIGH变为LOW，因为使用了上拉电阻）
            if (currentButtonState == LOW) {
                handleButtonPress();
            }
            // 按钮被松开（从LOW变为HIGH）
            else if (currentButtonState == HIGH) {
                handleButtonRelease();
            }
        }
    }
    
    lastButtonState = reading;
}

void handleButtonPress() {
    Serial.println("\n[BUTTON] *** 按钮被按下 - 开始录音 ***");
    
    // 如果已经在录音，不重复开始
    if (isRecording) {
        Serial.println("[BUTTON] 已在录音中，忽略按下事件");
        return;
    }
    
    // 检查WiFi连接
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[BUTTON] WiFi未连接，无法开始语音对话");
        // LED快速闪烁2次表示错误，不阻塞
        for (int i = 0; i < 2; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
        return;
    }
    
    // 检查AI服务是否初始化
    if (!aiInitialized) {
        Serial.println("[BUTTON] AI服务未初始化，无法开始对话");
        // LED快速闪烁2次表示服务未就绪
        for (int i = 0; i < 2; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
        return;
    }
    
    // 检查语音服务是否已初始化
    if (useRealtimeASR) {
        if (!realtimeASRInitialized) {
            Serial.println("[BUTTON] 实时识别服务未初始化，尝试重新初始化...");
            setupRealtimeASR();
            if (!realtimeASRInitialized) {
                Serial.println("[BUTTON] 实时识别仍未就绪，临时回退到RAW识别模式");
                useRealtimeASR = false;
            }
        }
    } else {
        if (!speechInitialized) {
            Serial.println("[BUTTON] 语音服务未初始化");
            // LED快速闪烁2次
            for (int i = 0; i < 2; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
            }
            return;
        }
    }
    
    // LED立即常亮表示正在录音
    digitalWrite(LED_PIN, HIGH);
    Serial.println("[LED] 录音开始，LED常亮");
    
    // 开始录音
    Serial.println("[BUTTON] 开始录音，请说话...");
    if (useRealtimeASR && realtimeASRInitialized) {
        startRealtimeRecording();  // 实时识别
    } else {
        // 使用RAW识别方式
        startVoiceRecording();
    }
}

void handleButtonRelease() {
    Serial.println("\n[BUTTON] *** 按钮被松开 - 停止录音 ***");
    
    // LED熄灭
    setLEDMode(LED_OFF);
    
    // 如果正在录音，停止录音并处理语音
    if (isRecording) {
        unsigned long recordingDuration = millis() - recordingStartTime;
        Serial.printf("[BUTTON] 实际录音时长: %lu ms\n", recordingDuration);
        
        Serial.println("[BUTTON] 停止录音并开始处理语音...");
        
        if (useRealtimeASR) {
            stopRealtimeRecording();  // 实时识别
        } else {
            stopVoiceRecording();     // 传统方式
        }
    } else {
        Serial.println("[BUTTON] 当前未在录音，忽略松开事件");
    }
}

void startVoiceConversation() {
    Serial.println("\n=== 语音对话开始 ===");
    
    // 检查语音服务是否已初始化
    if (!speechInitialized) {
        Serial.println("[VOICE] 语音服务未初始化，无法开始录音");
        // LED快速闪烁表示错误
        for (int i = 0; i < 5; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
        return;
    }
    
    // 开始真实的语音录音
    Serial.println("[VOICE] 开始录音...");
    startVoiceRecording();
    
    Serial.println("=== 等待用户松开按钮停止录音 ===\n");
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
    Serial.printf("  Button Pin (GPIO%d): %s\n", BUTTON_PIN, digitalRead(BUTTON_PIN) ? "HIGH (Not Pressed)" : "LOW (Pressed)");
    
    // 语音对话状态
    Serial.println("[VOICE]");
    Serial.printf("  Recording: %s\n", isRecording ? "✓ Active" : "✗ Inactive");
    Serial.printf("  Speech Service: %s\n", speechInitialized ? "✓ Ready" : "✗ Not initialized");
    
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
            // WiFi断开期间LED快闪（如果没有在录音中）
            if (!isRecording) {
                setLEDMode(LED_BLINK_FAST);
            }
        }
        
        // 尝试重连
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 5) {
            Serial.printf("[WIFI] 重连尝试 %d/5...", attempts + 1);
            WiFi.disconnect();
            // 在等待期间更新LED闪烁
            for (int i = 0; i < 10; i++) {
                updateLED();
                delay(100);
            }
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            
            // 等待连接，最多20秒
            int waitTime = 0;
            while (WiFi.status() != WL_CONNECTED && waitTime < 20) {
                // 等待期间保持LED快闪（若未在录音）
                for (int i = 0; i < 10; i++) {
                    updateLED();
                    delay(100);
                }
                Serial.print(".");
                waitTime++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\n[WIFI] ✓ 重连成功！");
                Serial.printf("[WIFI] IP地址: %s\n", WiFi.localIP().toString().c_str());
                wifiWasConnected = true;
                // WiFi重连成功，若未在录音，LED熄灭
                if (!isRecording) {
                    setLEDMode(LED_OFF);
                }
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
            // 确保稳定连接时LED熄灭（若未在录音）
            if (!isRecording) {
                setLEDMode(LED_OFF);
            }
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
    Serial.println("  button   - 测试按钮状态");
    Serial.println("  testbutton - 模拟按钮按下");
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
    Serial.println("  baidutoken - 测试百度API token获取");
    Serial.println("  record   - 开始语音录制");
    Serial.println("  stop     - 停止语音录制");
    Serial.println("  tts [文本] - 文本转语音");
    Serial.println("  ttsstream [文本] - 流式文本转语音");
    Serial.println("  testtone [频率] [时长] - 测试音频输出硬件");
    Serial.println("    例如: testtone 1000 2000 (播放1000Hz音调2秒)");
    Serial.println();
    Serial.println("硬件控制:");
    Serial.printf("  外部按钮: 连接GPIO%d到GND进行语音对话\n", BUTTON_PIN);
    Serial.println("  连接方式: GPIO1 ---- [按钮] ---- GND");
    Serial.println("  使用方法: 按下按钮开始录音，松开按钮停止录音（类似对讲机）");
    Serial.println("  LED指示: GPIO48显示系统状态");
    Serial.println("    - 常亮: 正在录音");
    Serial.println("    - 快闪: 正在播放/错误");
    Serial.println("    - 慢闪: 服务未就绪");
    Serial.println();
    Serial.println("直接输入中文即可与小智对话！");
    Serial.println("========================================");
    Serial.println("提示: 命令不区分大小写");
    Serial.println("提示: 系统每30秒显示一次心跳");
    Serial.println("========================================\n");
}

// 简单的HTTP测试函数
void testSimpleHTTP() {
    Serial.println("[HTTP_TEST] 测试基础HTTP连接...");
    
    HTTPClient http;
    WiFiClient client;
    
    // 测试一个简单的HTTP请求
    http.begin(client, "http://httpbin.org/get");
    http.addHeader("User-Agent", "ESP32-HTTPClient/1.0");
    
    int httpResponseCode = http.GET();
    Serial.printf("[HTTP_TEST] HTTP响应码: %d\n", httpResponseCode);
    
    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.printf("[HTTP_TEST] 响应长度: %d bytes\n", payload.length());
        if (payload.length() > 0) {
            String preview = payload.substring(0, min(200, (int)payload.length()));
            Serial.printf("[HTTP_TEST] 响应预览: %s...\n", preview.c_str());
        }
    } else {
        Serial.printf("[HTTP_TEST] 请求失败，错误码: %d\n", httpResponseCode);
    }
    
    http.end();
    Serial.println("[HTTP_TEST] 测试完成\n");
}

String chatWithDeepSeek(const String& message) {
    if (WiFi.status() != WL_CONNECTED) {
        return "网络未连接，请检查WiFi设置";
    }
    
    // 检查内存是否足够
    if (ESP.getFreeHeap() < 50000) {
        Serial.println("[DeepSeek] 内存不足，跳过此次请求");
        return "内存不足，请输入 'restart' 重启系统";
    }
    
    Serial.printf("[DeepSeek] 准备请求: %s\n", message.c_str());
    Serial.printf("[DeepSeek] 可用内存: %d bytes\n", ESP.getFreeHeap());
    
    // 使用低层WiFiClientSecure直接处理
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);
    
    Serial.println("[DeepSeek] 连接到 api.deepseek.com:443...");
    
    if (!client.connect("api.deepseek.com", 443)) {
        Serial.println("[DeepSeek] ✗ 无法连接到服务器");
        return "连接失败";
    }
    
    Serial.println("[DeepSeek] ✓ SSL连接建立成功");
    
    // 构建HTTP请求
    String request = "POST /v1/chat/completions HTTP/1.1\r\n";
    request += "Host: api.deepseek.com\r\n";
    request += "Authorization: Bearer sk-5f4807087b154707b6586f6eae1fc8c0\r\n";
    request += "Content-Type: application/json\r\n";
    request += "User-Agent: ESP32-XiaoZhi/1.0\r\n";
    request += "Connection: close\r\n";
    
    // 构建JSON请求体
    String jsonBody = "{";
    jsonBody += "\"model\":\"deepseek-chat\",";
    jsonBody += "\"messages\":[";
    jsonBody += "{\"role\":\"system\",\"content\":\"你是一个有用的AI助手。请简洁回答。\"},";
    jsonBody += "{\"role\":\"user\",\"content\":\"" + message + "\"}";
    jsonBody += "],";
    jsonBody += "\"max_tokens\":50,";
    jsonBody += "\"stream\":false";
    jsonBody += "}";
    
    request += "Content-Length: " + String(jsonBody.length()) + "\r\n";
    request += "\r\n";
    request += jsonBody;
    
    Serial.printf("[DeepSeek] 发送请求，长度: %d bytes\n", request.length());
    
    // 发送请求
    client.print(request);
    
    Serial.println("[DeepSeek] 等待响应...");
    
    // 读取响应状态行
    String statusLine = client.readStringUntil('\n');
    Serial.printf("[DeepSeek] 状态行: %s\n", statusLine.c_str());
    
    // 检查HTTP状态码
    bool isSuccess = statusLine.indexOf("200 OK") > 0;
    
    // 读取响应头
    String headers = "";
    int contentLength = -1;
    bool chunked = false;
    
    while (client.available() || client.connected()) {
        String line = client.readStringUntil('\n');
        line.trim();
        
        if (line.length() == 0) {
            break; // 空行，头部结束
        }
        
        headers += line + "\n";
        
        if (line.startsWith("Content-Length:")) {
            contentLength = line.substring(16).toInt();
            Serial.printf("[DeepSeek] Content-Length: %d\n", contentLength);
        }
        
        if (line.indexOf("chunked") > 0) {
            chunked = true;
            Serial.println("[DeepSeek] 使用分块传输编码");
        }
    }
    
    Serial.printf("[DeepSeek] 响应头读取完成，可用数据: %d bytes\n", client.available());
    
    // 读取响应体
    String responseBody = "";
    
    if (chunked) {
        // 处理分块传输
        while (client.available() || client.connected()) {
            String chunkSizeLine = client.readStringUntil('\n');
            chunkSizeLine.trim();
            
            if (chunkSizeLine.length() == 0) continue;
            
            int chunkSize = strtol(chunkSizeLine.c_str(), NULL, 16);
            Serial.printf("[DeepSeek] 块大小: %d bytes\n", chunkSize);
            
            if (chunkSize == 0) {
                break; // 最后一个块
            }
            
            char* buffer = new char[chunkSize + 1];
            int bytesRead = client.readBytes(buffer, chunkSize);
            buffer[bytesRead] = '\0';
            responseBody += String(buffer);
            delete[] buffer;
            
            client.readStringUntil('\n'); // 读取块后的CRLF
        }
    } else if (contentLength > 0) {
        // 使用Content-Length读取
        char* buffer = new char[contentLength + 1];
        int bytesRead = client.readBytes(buffer, contentLength);
        buffer[bytesRead] = '\0';
        responseBody = String(buffer);
        delete[] buffer;
    } else {
        // 读取直到连接关闭
        while (client.available() || client.connected()) {
            if (client.available()) {
                responseBody += client.readString();
            }
            delay(10);
        }
    }
    
    client.stop();
    
    Serial.printf("[DeepSeek] 响应体长度: %d bytes\n", responseBody.length());
    
    if (responseBody.length() > 0) {
        String preview = responseBody.substring(0, min(200, (int)responseBody.length()));
        Serial.printf("[DeepSeek] 响应预览: %s...\n", preview.c_str());
        
        // 解析JSON响应
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, responseBody);
        
        if (!error) {
            if (doc.containsKey("choices") && doc["choices"].size() > 0) {
                String aiResponse = doc["choices"][0]["message"]["content"].as<String>();
                conversationCount++;
                Serial.printf("[DeepSeek] AI回复: %s\n", aiResponse.c_str());
                return aiResponse;
            } else {
                Serial.println("[DeepSeek] JSON结构不正确，缺少choices字段");
                if (doc.containsKey("error")) {
                    String errorMsg = doc["error"]["message"].as<String>();
                    Serial.printf("[DeepSeek] API错误: %s\n", errorMsg.c_str());
                    return "API错误: " + errorMsg;
                }
                return "AI响应格式异常";
            }
        } else {
            Serial.printf("[DeepSeek] JSON解析失败: %s\n", error.c_str());
            return "JSON解析失败";
        }
    } else {
        Serial.println("[DeepSeek] ✗ 响应体为空");
        return "服务器返回空响应";
    }
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
    
    Serial.printf("[AI] 测试响应: %s\n", testResponse.c_str());
    
    // 改进的成功检测逻辑
    if (testResponse.indexOf("JSON解析失败") == -1 && 
        testResponse.indexOf("服务器连接失败") == -1 && 
        testResponse.indexOf("网络未连接") == -1 &&
        testResponse.indexOf("内存不足") == -1 &&
        testResponse.length() > 0) {
        aiInitialized = true;
        Serial.println("[AI] ✓ AI服务初始化成功！");
        
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
    
    // INMP441麦克风L/R引脚配置说明：
    // L/R接GND = 左声道，L/R接VDD = 右声道
    // 如果音频电平很低，尝试切换声道设置
    
    // 配置I2S音频输入 (INMP441麦克风专用配置)
    i2s_config_t i2s_config_in = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // INMP441使用32位
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // 尝试右声道（如果L/R接VDD）
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,   // 标准I2S格式
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,                            // DMA缓冲区数量
        .dma_buf_len = 1024,                           // 单个缓冲区大小（增大以提高吞吐量）
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config_in = {
        .bck_io_num = I2S_MIC_SCK_PIN,    // GPIO21 - SCK (串行时钟)
        .ws_io_num = I2S_MIC_WS_PIN,      // GPIO45 - WS (字选择/左右声道)
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SD_PIN     // GPIO47 - SD (串行数据)
    };
    
    Serial.printf("[AUDIO] I2S配置 - 采样率: %d, 位深: 32bit, DMA缓冲区: %d x %d\n", 
                  SAMPLE_RATE, i2s_config_in.dma_buf_count, i2s_config_in.dma_buf_len);
    Serial.printf("[AUDIO] 引脚配置 - SCK: %d, WS: %d, SD: %d\n", 
                  I2S_MIC_SCK_PIN, I2S_MIC_WS_PIN, I2S_MIC_SD_PIN);
    
    // 安装I2S驱动
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config_in, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ✗ I2S驱动安装失败: %d\n", err);
        return;
    }
    
    err = i2s_set_pin(I2S_NUM_0, &pin_config_in);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ✗ I2S引脚配置失败: %d\n", err);
        return;
    }
    
    // 设置I2S时钟
    err = i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_MONO);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ⚠️ I2S时钟设置警告: %d\n", err);
    }
    
    Serial.println("[AUDIO] ✓ I2S音频接口初始化成功");
    
    // 分配音频缓冲区（2秒录音，RAW格式无需Base64，大幅节省内存）
    // 2秒 * 16000 Hz * 2字节 = 64KB (RAW格式直接上传，无需额外33%编码开销)
    audioBufferSize = SAMPLE_RATE * 2 * 2; // 2秒的16位音频 = 64KB
    audioBuffer = (uint8_t*)malloc(audioBufferSize);
    if (!audioBuffer) {
        Serial.println("[AUDIO] ✗ 音频缓冲区分配失败");
        return;
    }
    
    Serial.printf("[AUDIO] 音频缓冲区: %d bytes (%.1f KB) - RAW格式优化\n", audioBufferSize, audioBufferSize / 1024.0);
    Serial.printf("[AUDIO] 可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
}

void setupBaiduSpeech() {
    Serial.println("[SPEECH] 初始化百度语音服务...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[SPEECH] WiFi未连接，跳过语音服务初始化");
        return;
    }
    
    Serial.println("[BAIDU] 开始获取access token...");
    Serial.printf("[BAIDU] APP_ID: %s\n", BAIDU_APP_ID);
    Serial.printf("[BAIDU] API_KEY: %s\n", String(BAIDU_API_KEY).substring(0, 8).c_str());
    Serial.printf("[BAIDU] SECRET_KEY: %s\n", String(BAIDU_SECRET_KEY).substring(0, 8).c_str());
    
    // 初始化百度语音服务
    if (baiduSpeech.begin(BAIDU_APP_ID, BAIDU_API_KEY, BAIDU_SECRET_KEY)) {
        speechInitialized = true;
        Serial.println("[SPEECH] ✓ 百度语音服务初始化成功");
        
        // 配置语音识别和合成参数
        baiduSpeech.setASRConfig("zh", SAMPLE_RATE);
        baiduSpeech.setTTSConfig("zh", 7, 5, 15); // 语速7(加快), 音调5, 音量15(最大)
        
        // 成功指示
        for (int i = 0; i < 2; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(150);
            digitalWrite(LED_PIN, LOW);
            delay(150);
        }
    } else {
        Serial.println("[BAIDU] 百度语音服务初始化失败");
        String lastError = baiduSpeech.getLastError();
        Serial.printf("[SPEECH] 错误: %s\n", lastError.c_str());
        
        // 添加更详细的错误分析
        if (lastError.indexOf("access token") != -1) {
            Serial.println("[BAIDU] 错误分析: access token获取失败");
            Serial.println("[BAIDU] 可能原因:");
            Serial.println("[BAIDU] 1. API密钥错误");
            Serial.println("[BAIDU] 2. 网络连接问题");
            Serial.println("[BAIDU] 3. 百度服务器问题");
        }
        
        // 错误指示
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(80);
            digitalWrite(LED_PIN, LOW);
            delay(80);
        }
    }
}

// 直接测试百度API的access token获取
void testBaiduTokenAPI() {
    Serial.println("[BAIDU_TEST] 测试百度API access token获取...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[BAIDU_TEST] WiFi未连接");
        return;
    }
    
    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    
    String tokenUrl = "https://aip.baidubce.com/oauth/2.0/token";
    tokenUrl += "?grant_type=client_credentials";
    tokenUrl += "&client_id=" + String(BAIDU_API_KEY);
    tokenUrl += "&client_secret=" + String(BAIDU_SECRET_KEY);
    
    Serial.printf("[BAIDU_TEST] 请求URL: %s\n", tokenUrl.c_str());
    
    http.begin(client, tokenUrl);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.POST("");
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.printf("[BAIDU_TEST] ✓ Token请求成功: %s\n", response.c_str());
        
        // 解析token - 增加缓冲区大小
        DynamicJsonDocument doc(4096); // 增加到4KB
        DeserializationError error = deserializeJson(doc, response);
        if (!error) {
            if (doc.containsKey("access_token")) {
                String token = doc["access_token"].as<String>();
                Serial.printf("[BAIDU_TEST] ✓ Access Token: %s...\n", token.substring(0, 20).c_str());
                
                // 显示token的完整长度和过期时间
                if (doc.containsKey("expires_in")) {
                    int expiresIn = doc["expires_in"].as<int>();
                    Serial.printf("[BAIDU_TEST] ✓ Token长度: %d, 有效期: %d秒\n", token.length(), expiresIn);
                }
            } else {
                Serial.println("[BAIDU_TEST] ✗ 响应中没有access_token字段");
            }
        } else {
            Serial.printf("[BAIDU_TEST] ✗ Token解析失败: %s\n", error.c_str());
            Serial.printf("[BAIDU_TEST] 响应长度: %d bytes\n", response.length());
        }
    } else {
        String response = http.getString();
        Serial.printf("[BAIDU_TEST] ✗ Token请求失败: %d\n", httpCode);
        Serial.printf("[BAIDU_TEST] 错误响应: %s\n", response.c_str());
    }
    
    http.end();
    client.stop();
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

// 初始化实时语音识别
void setupRealtimeASR() {
    Serial.println("[REALTIME-ASR] 初始化实时语音识别...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[REALTIME-ASR] WiFi未连接，跳过初始化");
        return;
    }
    
    // 初始化实时识别客户端
    if (realtimeASR.begin(BAIDU_APP_ID, BAIDU_API_KEY)) {
        Serial.println("[REALTIME-ASR] ✓ 初始化成功");
        
        // 设置回调函数
        realtimeASR.onPartialResult(onRealtimePartialResult);
        realtimeASR.onFinalResult(onRealtimeFinalResult);
        realtimeASR.onError(onRealtimeError);
        realtimeASR.onConnected(onRealtimeConnected);
        realtimeASR.onDisconnected(onRealtimeDisconnected);
        
        realtimeASRInitialized = true;
        
        // 成功指示 - 3次快速闪烁
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    } else {
        Serial.println("[REALTIME-ASR] ✗ 初始化失败");
        realtimeASRInitialized = false;
    }
}

// 实时识别回调：临时结果
void onRealtimePartialResult(const String& result) {
    Serial.printf("[实时] %s\n", result.c_str());
    // 不再在部分结果时操作LED，保持“按下=常亮，松开=熄灭”的规则
}

// 实时识别回调：最终结果
void onRealtimeFinalResult(const String& result, uint32_t startTime, uint32_t endTime) {
    Serial.printf("[最终] %s [%d-%d ms]\n", result.c_str(), startTime, endTime);
    
    // 累积识别结果
    if (fullRecognizedText.length() > 0) {
        fullRecognizedText += " ";
    }
    fullRecognizedText += result;
    
    Serial.printf("[完整识别] %s\n", fullRecognizedText.c_str());
}

// 实时识别回调：错误
void onRealtimeError(int errNo, const String& errMsg) {
    Serial.printf("[错误] %d: %s\n", errNo, errMsg.c_str());
    
    // LED快速闪烁表示错误
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
        delay(50);
    }
}

// 实时识别回调：已连接
void onRealtimeConnected() {
    Serial.println("[实时识别] WebSocket已连接，可以开始说话");
    // 按规则：连接成功后LED保持当前模式（通常为：未按键=熄灭；按着键=常亮）
}

// 实时识别回调：断开连接
void onRealtimeDisconnected() {
    Serial.println("[实时识别] WebSocket已断开");
    // 不在此强制改动LED，保持按钮释放逻辑来控制熄灭
}

// 开始实时录音
void startRealtimeRecording() {
    Serial.println("[REALTIME] 开始实时录音...");
    
    if (!realtimeASRInitialized) {
        Serial.println("[REALTIME] ✗ 实时识别未初始化");
        return;
    }
    
    if (isRecording) {
        Serial.println("[REALTIME] ✗ 已在录音中");
        return;
    }
    
    // 清空之前的识别结果
    fullRecognizedText = "";
    
    // 先建立WebSocket连接，再开始录音，避免开头丢失
    Serial.println("[REALTIME] 正在建立WebSocket连接...");
    if (!realtimeASR.connect()) {
        Serial.println("[REALTIME] ✗ WebSocket连接失败，无法开始录音");
        Serial.println(realtimeASR.getLastError());
        // LED快速闪烁表示连接失败
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(80);
            digitalWrite(LED_PIN, LOW);
            delay(80);
        }
        return;
    }
    
    Serial.println("[REALTIME] ✓ WebSocket连接成功");
    
    // WebSocket连接成功后，立即开始录音
    isRecording = true;
    recordingStartTime = millis();
    recordedSize = 0;
    
    Serial.printf("[REALTIME] 录音已开始，开始实时传输音频\n");
    Serial.printf("[REALTIME] isRecording=%d, recordingStartTime=%lu ms\n", 
                  isRecording, recordingStartTime);
}

// 停止实时录音
void stopRealtimeRecording() {
    Serial.println("[REALTIME] 停止实时录音...");
    
    if (!isRecording) {
        Serial.println("[REALTIME] 未在录音");
        return;
    }
    
    // 立即标记为非录音状态，防止重复触发
    isRecording = false;
    
    // 录音停止后立即关闭LED
    digitalWrite(LED_PIN, LOW);
    Serial.println("[LED] 录音停止，LED已关闭");
    
    // 发送结束帧
    realtimeASR.finish();
    
    // 等待ASR最终结果
    unsigned long startWait = millis();
    while (millis() - startWait < 2000) {
        realtimeASR.loop();
        delay(10);
    }
    
    // 断开连接
    realtimeASR.disconnect();
    
    unsigned long duration = millis() - recordingStartTime;
    Serial.printf("[REALTIME] 录音时长: %lu ms\n", duration);
    Serial.printf("[REALTIME] 完整识别结果: %s\n", fullRecognizedText.c_str());
    
    // 如果有识别结果，立即发送给AI
    if (fullRecognizedText.length() > 0) {
        Serial.println("[AI] 正在生成回复...");
        
        // AI处理中不开LED，保持关闭状态
        String aiResponse = chatWithDeepSeek(fullRecognizedText);
        
        if (aiResponse.length() > 0) {
            Serial.printf("[AI] ✓ 回复: %s\n", aiResponse.c_str());
            
            // TTS播放时LED保持关闭
            Serial.println("[TTS] 流式合成并播放AI回复...");
            unsigned long ttsStart = millis();
            
            bool ttsSuccess = speakTextStream(aiResponse);
            
            unsigned long ttsEnd = millis();
            Serial.printf("[TTS] 播放耗时: %lu ms\n", ttsEnd - ttsStart);
            
            if (ttsSuccess) {
                // 播放完成后LED快速闪烁3次表示对话完成
                Serial.println("[LED] 播放完成，LED闪烁提示");
                for (int i = 0; i < 3; i++) {
                    digitalWrite(LED_PIN, HIGH);
                    delay(150);
                    digitalWrite(LED_PIN, LOW);
                    delay(150);
                }
            } else {
                Serial.println("[TTS] 播放失败");
            }
        } else {
            Serial.println("[AI] AI回复失败");
        }
        
        // 清空识别结果，准备下次对话
        fullRecognizedText = "";
    } else {
        Serial.println("[REALTIME] 未识别到语音内容");
        // 未识别到内容，快速闪烁2次
        for (int i = 0; i < 2; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
    }
    
    // 最后确保LED关闭和状态清理
    digitalWrite(LED_PIN, LOW);
    recordingStartTime = 0;
    
    Serial.println("[REALTIME] 对话流程完成，系统就绪");
}

void startVoiceRecording() {
    Serial.println("[RECORD] 进入startVoiceRecording函数");
    
    if (!speechInitialized) {
        Serial.println("[RECORD] ✗ 语音服务未初始化");
        // LED快速闪烁表示服务未就绪
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(100);
            digitalWrite(LED_PIN, LOW);
            delay(100);
        }
        return;
    }
    
    if (isRecording) {
        Serial.println("[RECORD] ✗ 已在录音中");
        return;
    }
    
    // 检查音频缓冲区
    if (audioBuffer == nullptr) {
        Serial.println("[RECORD] ✗ 音频缓冲区未初始化！");
        Serial.println("[RECORD] 尝试重新初始化音频系统...");
        setupAudio();
        
        if (audioBuffer == nullptr) {
            Serial.println("[RECORD] ✗ 音频缓冲区重新初始化失败！");
            // LED快速闪烁表示错误
            for (int i = 0; i < 5; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(80);
                digitalWrite(LED_PIN, LOW);
                delay(80);
            }
            return;
        } else {
            Serial.println("[RECORD] ✓ 音频缓冲区重新初始化成功");
        }
    }
    
    Serial.println("[RECORD] 开始语音录制...");
    isRecording = true;
    recordedSize = 0;
    recordingStartTime = millis();  // 正确初始化录音开始时间
    
    Serial.printf("[RECORD] 录音开始时间戳: %lu ms\n", recordingStartTime);
    Serial.printf("[RECORD] 当前millis(): %lu ms\n", millis());
    Serial.printf("[RECORD] 音频缓冲区: %p, 大小: %d bytes\n", audioBuffer, audioBufferSize);
    
    // 清空音频缓冲区
    memset(audioBuffer, 0, audioBufferSize);
    Serial.printf("[RECORD] 已清空音频缓冲区: %d bytes\n", audioBufferSize);
    
    // 清空I2S缓冲区
    i2s_zero_dma_buffer(I2S_NUM_0);
    
    // 跳过开头的一些数据（可能包含噪音）
    size_t bytes_read = 0;
    int32_t temp_buffer[512]; // 增加缓冲区大小
    // 跳过更多开头数据以确保稳定
    for (int i = 0; i < 5; i++) {
        esp_err_t result = i2s_read(I2S_NUM_0, temp_buffer, sizeof(temp_buffer), &bytes_read, 200);
        Serial.printf("[RECORD] 跳过噪音数据 %d: %d bytes, 结果: %d\n", i+1, bytes_read, result);
    }
    
    // LED指示录音状态 - 点亮LED表示正在录音
    digitalWrite(LED_PIN, HIGH);
    Serial.println("[RECORD] ✓ LED已点亮，表示正在录音");
    
    Serial.println("[RECORD] ✓ 录音开始，请说话...");
    Serial.println("[RECORD] 松开按钮停止录音");
    Serial.println("[RECORD] 💡 建议录音时间1-3秒以获得最佳识别效果");
    Serial.println("[RECORD] 🎤 请对着麦克风大声清晰地说话，距离10-20cm");
}

void stopVoiceRecording() {
    if (!isRecording) {
        Serial.println("[RECORD] 当前未在录音");
        return;
    }
    
    // 计算录音时长
    unsigned long recordingDuration = 0;
    if (recordingStartTime > 0) {
        recordingDuration = millis() - recordingStartTime;
        Serial.printf("[RECORD] 录音时长: %lu 毫秒 (%.1f秒)\n", recordingDuration, recordingDuration / 1000.0);
    } else {
        Serial.println("[RECORD] ⚠️ 录音开始时间异常");
    }
    
    isRecording = false;
    recordingStartTime = 0;  // 重置录音开始时间
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("[RECORD] 录音停止");
    Serial.printf("[RECORD] 录制音频大小: %d bytes\n", recordedSize);
    
    if (recordedSize > 0) {
        processVoiceInput();
    } else {
        Serial.println("[RECORD] ⚠️ 没有录制到音频数据，可能原因：");
        Serial.println("    1. 麦克风连接问题");
        Serial.println("    2. I2S配置问题");
        Serial.println("    3. 录音时间太短");
        Serial.println("    4. 音频缓冲区问题");
    }
}

void processVoiceInput() {
    if (!speechInitialized || recordedSize == 0) {
        Serial.println("[PROCESS] 无法处理语音输入");
        return;
    }
    
    Serial.printf("[PROCESS] 开始语音识别...（音频大小: %d bytes）\n", recordedSize);
    Serial.printf("[PROCESS] 当前可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    // 检查音频数据最小要求（至少0.5秒的音频）
    size_t minAudioSize = SAMPLE_RATE * 1; // 0.5秒的16位音频
    if (recordedSize < minAudioSize) {
        Serial.printf("[PROCESS] ⚠️ 音频较短（%d bytes < %d bytes），但尝试继续识别\n", recordedSize, minAudioSize);
    }
    
    // 验证音频缓冲区指针
    if (recordBuffer == nullptr || audioBuffer == nullptr) {
        Serial.println("[PROCESS] ✗ 音频缓冲区指针无效");
        return;
    }
    
    // 检查音频数据是否有效
    Serial.println("[PROCESS] 验证音频数据...");
    bool hasNonZeroData = false;
    uint32_t sum = 0;
    uint16_t maxLevel = 0;
    uint16_t* samples = (uint16_t*)recordBuffer;
    size_t sampleCount = recordedSize / 2;
    
    for (size_t i = 0; i < sampleCount; i++) {
        uint16_t level = abs((int16_t)samples[i]);
        if (samples[i] != 0) {
            hasNonZeroData = true;
        }
        sum += level;
        if (level > maxLevel) maxLevel = level;
    }
    
    uint16_t avgLevel = sampleCount > 0 ? sum / sampleCount : 0;
    Serial.printf("[PROCESS] 音频统计 - 平均电平: %d, 最大电平: %d\n", avgLevel, maxLevel);
    
    // 音频质量评估
    if (maxLevel < 50) {
        Serial.println("[PROCESS] ⚠️ 音频电平较低，建议靠近麦克风说话");
    } else if (maxLevel > 20000) {
        Serial.println("[PROCESS] ⚠️ 音频电平过高，可能失真");
    } else {
        Serial.println("[PROCESS] ✓ 音频电平正常");
    }
    
    if (!hasNonZeroData) {
        Serial.println("[PROCESS] ✗ 音频数据全为零，麦克风或I2S配置问题");
        return;
    }
    
    // 使用RAW格式直接上传，无需Base64编码，大幅节省内存
    Serial.println("[PROCESS] 使用RAW格式识别（无Base64编码，节省33%内存）...");
    String recognizedText = baiduSpeech.recognizeSpeechRaw((uint8_t*)recordBuffer, recordedSize, SAMPLE_RATE);
    
    if (recognizedText.length() > 0) {
        Serial.printf("[PROCESS] ✓ 识别结果: '%s'\n", recognizedText.c_str());
        
        // 检查识别结果是否过短
        if (recognizedText.length() <= 2) {
            Serial.println("[PROCESS] ⚠️ 识别结果较短，可能不完整，建议：");
            Serial.println("    1. 录音时间适当延长到2-3秒");
            Serial.println("    2. 说话更清晰，语速适中");
            Serial.println("    3. 确保环境安静，减少背景噪音");
        }
        
        // 与AI对话
        Serial.println("[PROCESS] 发送给AI...");
        String aiResponse = chatWithDeepSeek(recognizedText);
        
        Serial.printf("[PROCESS] AI回复: %s\n", aiResponse.c_str());
        
        // 流式文本转语音并直接I2S播放
        Serial.println("[PROCESS] 流式合成并播放AI回复...");
        speakTextStream(aiResponse);
    } else {
        Serial.printf("[PROCESS] 语音识别失败: %s\n", baiduSpeech.getLastError().c_str());
    }
    
    // 清空录音缓冲区
    recordedSize = 0;
}

void playTTSAudio(uint8_t* audioData, size_t audioSize) {
    // 兼容旧接口（非流式）：如果是WAV/PCM则直接写I2S
    Serial.printf("[AUDIO] 播放音频: %d bytes\n", audioSize);
    if (!audioData || audioSize == 0) return;
    if (!ensureSpeakerI2S()) return;
    size_t offset = 0;
    if (audioSize >= 44 && memcmp(audioData, "RIFF", 4) == 0) {
        offset = 44; // 跳过WAV头
    }
    writePCMToI2S(audioData + offset, audioSize - offset);
    Serial.println("[AUDIO] 音频播放完成");
}

// 初始化I2S扬声器（MAX98357）
static bool ensureSpeakerI2S() {
    if (speakerInitialized) return true;
    
    Serial.println("[SPK] ====== 开始初始化I2S扬声器 ======");
    Serial.printf("[SPK] GPIO配置: BCLK=%d, LRCLK=%d, DIN=%d\n", 
                  I2S_SPK_SCK_PIN, I2S_SPK_WS_PIN, I2S_SPK_SD_PIN);
    Serial.printf("[SPK] 采样率: %d Hz\n", SAMPLE_RATE);
    
    // MAX98357A专用I2S配置 - 使用立体声模式
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // 立体声
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    Serial.println("[SPK] 安装I2S驱动...");
    esp_err_t err = i2s_driver_install(I2S_NUM_1, &cfg, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[SPK] ✗ I2S驱动安装失败: %d\n", err);
        return false;
    }
    Serial.println("[SPK] ✓ I2S驱动安装成功");
    
    i2s_pin_config_t pins = {
        .bck_io_num = I2S_SPK_SCK_PIN,
        .ws_io_num = I2S_SPK_WS_PIN,
        .data_out_num = I2S_SPK_SD_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    Serial.println("[SPK] 配置I2S引脚...");
    err = i2s_set_pin(I2S_NUM_1, &pins);
    if (err != ESP_OK) {
        Serial.printf("[SPK] ✗ I2S引脚配置失败: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_1);
        return false;
    }
    Serial.println("[SPK] ✓ I2S引脚配置成功");
    
    Serial.println("[SPK] 设置I2S时钟...");
    err = i2s_set_clk(I2S_NUM_1, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    if (err != ESP_OK) {
        Serial.printf("[SPK] ⚠ I2S时钟设置返回: %d\n", err);
    }
    
    // 清空DMA缓冲区
    i2s_zero_dma_buffer(I2S_NUM_1);
    
    speakerInitialized = true;
    Serial.println("[SPK] ====== ✓ 扬声器初始化完成 (立体声模式) ======\n");
    return true;
}

// 初始化扬声器并发送静音数据以消除初始噪音
void initializeSpeakerAndSilence() {
    Serial.println("[SPK-INIT] 初始化扬声器...");
    
    if (!ensureSpeakerI2S()) {
        Serial.println("[SPK-INIT] ✗ 扬声器初始化失败");
        return;
    }
    
    Serial.println("[SPK-INIT] 发送静音数据以消除噪音...");
    
    // 创建500ms静音数据 (16kHz * 0.5s * 2字节/样本 * 2声道)
    const size_t silenceDuration = SAMPLE_RATE / 2; // 0.5秒
    const size_t silenceSize = silenceDuration * 2 * 2; // 立体声16位
    int16_t* silenceBuffer = (int16_t*)calloc(silenceDuration * 2, sizeof(int16_t));
    
    if (silenceBuffer) {
        size_t bytesWritten = 0;
        esp_err_t err = i2s_write(I2S_NUM_1, silenceBuffer, silenceSize, &bytesWritten, portMAX_DELAY);
        
        if (err == ESP_OK) {
            Serial.printf("[SPK-INIT] ✓ 已发送 %d 字节静音数据，消除初始噪音\n", bytesWritten);
        } else {
            Serial.printf("[SPK-INIT] ⚠ 静音数据写入失败: %d\n", err);
        }
        
        free(silenceBuffer);
    } else {
        Serial.println("[SPK-INIT] ⚠ 无法分配静音缓冲区");
    }
    
    Serial.println("[SPK-INIT] ✓ 扬声器初始化完成，噪音已消除\n");
}

// 将单声道16-bit PCM写入I2S，扩展为双声道输出
static bool writePCMToI2S(const uint8_t* data, size_t len) {
    if (!speakerInitialized) return false;
    if (!data || len == 0) return true;
    const int16_t* in = (const int16_t*)data;
    size_t samples = len / 2;
    const size_t chunk = 256; // 每批处理样本数
    int16_t stereoBuf[chunk * 2];
    size_t processed = 0;
    while (processed < samples) {
        size_t n = min(chunk, samples - processed);
        for (size_t i = 0; i < n; ++i) {
            int16_t s = in[processed + i];
            // 增加3倍增益（注意防止溢出）
            int32_t amplified = (int32_t)s * 3;
            if (amplified > 32767) amplified = 32767;
            if (amplified < -32768) amplified = -32768;
            s = (int16_t)amplified;
            stereoBuf[2 * i] = s;      // 左
            stereoBuf[2 * i + 1] = s;  // 右
        }
        size_t bytesToWrite = n * 2 * sizeof(int16_t);
        size_t written = 0;
        esp_err_t err = i2s_write(I2S_NUM_1, (const void*)stereoBuf, bytesToWrite, &written, portMAX_DELAY);
        if (err != ESP_OK) {
            Serial.printf("[SPK] 写入失败: %d\n", err);
            return false;
        }
        processed += n;
    }
    return true;
}

// 流式文本转语音并播放（HTTP流式读取 → I2S写入）
bool speakTextStream(const String& text) {
    if (text.length() == 0) return false;
    if (!ensureSpeakerI2S()) return false;

    Serial.println("[TTS] 开始流式合成...");
    // 跳过一次性WAV头（若存在）
    struct HeaderSkipper {
        bool skipped = false;
        size_t remaining = 44;
        void apply(uint8_t*& p, size_t& n) {
            if (skipped) return;
            if (n >= remaining) { p += remaining; n -= remaining; skipped = true; remaining = 0; }
            else { remaining -= n; n = 0; }
        }
    } skipper;

    bool ok = baiduSpeech.synthesizeSpeechStream(text,
        [&](const uint8_t* chunk, size_t len) -> bool {
            if (len == 0) return true;
            uint8_t* p = (uint8_t*)chunk;
            size_t n = len;
            // 若前缀是RIFF，启动跳过逻辑
            if (!skipper.skipped && len >= 4 && memcmp(p, "RIFF", 4) == 0) {
                // 触发后续累计跳过44字节
            }
            if (!skipper.skipped) {
                skipper.apply(p, n);
                if (n == 0) return true;
            }
            return writePCMToI2S(p, n);
        }, SAMPLE_RATE, 6);

    // 数据传输完成后，等待I2S DMA缓冲区播放完成
    // DMA缓冲区大小：8个 × 256样本 × 2声道 × 2字节 = 8192字节
    // 播放时间：8192字节 / (16000Hz × 2声道 × 2字节/样本) = 0.128秒
    // 为安全起见等待200ms确保播放完成
    if (ok) {
        delay(200);
        Serial.println("[TTS] ✓ 播放完成");
    } else {
        Serial.printf("[TTS] ✗ 播放失败: %s\n", baiduSpeech.getLastError().c_str());
    }
    return ok;
}

// 测试音调生成函数（用于硬件测试，使用分块播放节省内存）
void playTestTone(int frequency, int durationMs) {
    Serial.println("\n[TONE] ====== 开始测试音调 ======");
    
    if (!ensureSpeakerI2S()) {
        Serial.println("[TONE] ✗ 扬声器未初始化");
        return;
    }
    
    Serial.printf("[TONE] 频率: %d Hz\n", frequency);
    Serial.printf("[TONE] 持续时间: %d ms\n", durationMs);
    Serial.printf("[TONE] 当前可用内存: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[TONE] I2S端口: I2S_NUM_1\n");
    Serial.printf("[TONE] GPIO引脚: BCLK=%d, LRCLK=%d, DIN=%d\n",
                  I2S_SPK_SCK_PIN, I2S_SPK_WS_PIN, I2S_SPK_SD_PIN);
    
    const int sampleRate = 16000;
    const int amplitude = 32767; // 最大振幅
    const int chunkSize = 512; // 每次生成512个样本（约32ms）
    int16_t toneBuffer[chunkSize * 2]; // 立体声缓冲区
    
    int totalSamples = (sampleRate * durationMs) / 1000;
    int samplesGenerated = 0;
    size_t totalBytesWritten = 0;
    
    Serial.printf("[TONE] 开始生成并播放 %d 个样本 (立体声)...\n", totalSamples);
    
    while (samplesGenerated < totalSamples) {
        int samplesToGenerate = min(chunkSize, totalSamples - samplesGenerated);
        
        // 生成一块正弦波
        for (int i = 0; i < samplesToGenerate; i++) {
            float t = (float)(samplesGenerated + i) / sampleRate;
            int16_t sample = (int16_t)(amplitude * sin(2.0 * PI * frequency * t));
            toneBuffer[i * 2] = sample;      // 左声道
            toneBuffer[i * 2 + 1] = sample;  // 右声道
        }
        
        // 写入I2S（立体声）
        size_t bytesWritten;
        size_t bytesToWrite = samplesToGenerate * 2 * sizeof(int16_t);
        esp_err_t err = i2s_write(I2S_NUM_1, toneBuffer, bytesToWrite, &bytesWritten, portMAX_DELAY);
        
        if (err != ESP_OK) {
            Serial.printf("[TONE] ✗ I2S写入失败: %d (已写入 %d bytes)\n", err, totalBytesWritten);
            return;
        }
        
        if (bytesWritten != bytesToWrite) {
            Serial.printf("[TONE] ⚠ 写入不完整: %d/%d bytes\n", bytesWritten, bytesToWrite);
        }
        
        totalBytesWritten += bytesWritten;
        samplesGenerated += samplesToGenerate;
        
        // 每处理1/4显示进度
        if (samplesGenerated % (totalSamples / 4) == 0 && samplesGenerated > 0) {
            Serial.printf("[TONE] 进度: %d%%\n", (samplesGenerated * 100) / totalSamples);
        }
    }
    
    Serial.printf("[TONE] ✓ 播放完成，总共写入 %d bytes (%d 样本)\n", totalBytesWritten, samplesGenerated);
    Serial.println("[TONE] ====== 测试结束 ======\n");
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
    
    // Initialize Button
    Serial.println("[BUTTON] Initializing button control...");
    setupButton();
    Serial.println("[BUTTON] Button initialization complete");
    
    // LED startup indication
    Serial.println("[LED] Running startup LED test...");
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
    
    // 初始化音频系统（在WiFi之前初始化，确保硬件就绪）
    Serial.println("[AUDIO] 开始音频系统初始化...");
    setupAudio();
    
    // 初始化扬声器并消除初始噪音
    Serial.println("[SPEAKER] 初始化扬声器并消除初始噪音...");
    initializeSpeakerAndSilence();
    
    // Connect WiFi
    Serial.println("[WIFI] Starting WiFi connection...");
    Serial.printf("[WIFI] SSID: %s\n", WIFI_SSID);
    Serial.println("[WIFI] Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // LED快闪表示连接中
    setLEDMode(LED_BLINK_FAST);
    
    int attempts = 0;
    unsigned long wifiStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(100);
        updateLED();  // 更新LED状态
        
        if (attempts % 10 == 0) {
            Serial.print(".");
        }
        attempts++;
        
        // Show progress every 5 seconds
        if (attempts % 50 == 0) {
            Serial.printf("\n[WIFI] Connection attempt %d/300 (%.1fs)...", attempts, attempts/10.0);
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
        
        // WiFi连接成功，LED熄灭
        setLEDMode(LED_OFF);
        
        // 初始化AI服务
        Serial.println("[AI] 开始AI服务初始化...");
        setupAI();
        
        // 初始化语音服务
        Serial.println("[SPEECH] 开始语音服务初始化...");
        setupBaiduSpeech();
        
        // 初始化实时语音识别
        if (useRealtimeASR) {
            Serial.println("[REALTIME-ASR] 开始实时识别初始化...");
            setupRealtimeASR();
        }
        
        // 检查所有服务是否初始化成功
        if (aiInitialized && speechInitialized && realtimeASRInitialized) {
            systemFullyInitialized = true;
            Serial.println("\n[SYSTEM] ✓ 所有服务初始化完成！");
            
            // 播放初始化完成提示音
            delay(500);  // 稍等让系统稳定
            Serial.println("[SYSTEM] 播放初始化完成提示...");
            speakTextStream("初始化已完成，现在可以开始了");
            
            // LED快速闪烁5次表示系统就绪
            for (int i = 0; i < 5; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(100);
                digitalWrite(LED_PIN, LOW);
                delay(100);
            }
        } else {
            systemFullyInitialized = false;
            Serial.println("\n[SYSTEM] ⚠ 部分服务初始化失败");
            Serial.printf("[SYSTEM] AI: %s, Speech: %s, RealtimeASR: %s\n",
                         aiInitialized ? "✓" : "✗",
                         speechInitialized ? "✓" : "✗",
                         realtimeASRInitialized ? "✓" : "✗");
        }
        
    } else {
        Serial.println("\n[WIFI] ✗ WiFi connection failed!");
        Serial.printf("[WIFI] Status code: %d\n", WiFi.status());
        Serial.println("[WIFI] Please check SSID and password in config.h");
        Serial.println("[WIFI] 音频系统已初始化，但网络服务不可用");
        
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
    
    // 更新LED状态（非阻塞），确保闪烁/常亮及时生效
    updateLED();
    
    // 检查按钮状态（高频率检查以确保响应性）
    checkButton();
    
    // WebSocket循环处理（实时识别模式）- 必须在音频处理之前
    if (useRealtimeASR && realtimeASRInitialized) {
        realtimeASR.loop();
    }
    
    // 检查录音超时（防止按钮卡住导致无限录音）
    if (isRecording && recordingStartTime > 0) {
        // 重新获取当前时间，避免竞态条件
        currentTime = millis();
        
        // 检查时间是否合理（防止下溢）
        if (currentTime >= recordingStartTime) {
            unsigned long recordingDuration = currentTime - recordingStartTime;
            if (recordingDuration > MAX_RECORD_TIME) {
                Serial.println("[RECORD] ⚠️ 录音超时，自动停止录音");
                Serial.printf("[RECORD] 当前时间: %lu ms, 开始时间: %lu ms\n", currentTime, recordingStartTime);
                Serial.printf("[RECORD] 录音时长: %lu 毫秒 (%.1f秒)\n", recordingDuration, recordingDuration / 1000.0);
                Serial.printf("[RECORD] 最大录音时间: %d ms\n", MAX_RECORD_TIME);
                
                // 根据模式调用正确的停止函数
                if (useRealtimeASR) {
                    stopRealtimeRecording();
                } else {
                    stopVoiceRecording();
                }
            }
        } else {
            // 时间异常，可能是刚刚开始录音
            Serial.printf("[RECORD] 时间异常 - 当前: %lu, 开始: %lu\n", currentTime, recordingStartTime);
        }
    }
    
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
            
            // 重新初始化百度语音服务
            if (!speechInitialized) {
                Serial.println("[SPEECH] WiFi重连成功，尝试初始化百度语音服务...");
                setupBaiduSpeech();
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
            
            if (command == "button" || command == "btn") {
                Serial.println("[BUTTON] 按钮状态测试:");
                Serial.printf("  GPIO%d 当前状态: %s\n", BUTTON_PIN, digitalRead(BUTTON_PIN) ? "HIGH (未按下)" : "LOW (按下)");
                Serial.printf("  上次状态: %s\n", lastButtonState ? "HIGH" : "LOW");
                Serial.printf("  当前确认状态: %s\n", currentButtonState ? "HIGH" : "LOW");
                Serial.printf("  防抖延时: %d ms\n", BUTTON_DEBOUNCE);
                Serial.println("  请按下Boot按钮测试...");
                return;
            }
            
            if (command == "testbutton") {
                Serial.println("[BUTTON] 模拟按钮按下测试...");
                handleButtonPress();
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
            } else if (command == "baidutoken") {
                testBaiduTokenAPI();
            } else if (command == "httptest") {
                testSimpleHTTP();
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
            } else if (command.startsWith("ttsstream ")) {
                // 流式文本转语音测试
                String text = input.substring(String("ttsstream ").length());
                if (speechInitialized) {
                    Serial.printf("TTS Stream: %s\n", text.c_str());
                    speakTextStream(text);
                } else {
                    Serial.println("语音服务未初始化");
                }
            } else if (command == "testtone" || command.startsWith("testtone ")) {
                // 测试音调（用于硬件诊断）
                int freq = 1000; // 默认1000Hz
                int duration = 1000; // 默认1秒
                
                if (command.startsWith("testtone ")) {
                    String params = input.substring(String("testtone ").length());
                    int spaceIdx = params.indexOf(' ');
                    if (spaceIdx > 0) {
                        freq = params.substring(0, spaceIdx).toInt();
                        duration = params.substring(spaceIdx + 1).toInt();
                    } else {
                        freq = params.toInt();
                    }
                }
                
                Serial.printf("播放测试音: %dHz, %dms\n", freq, duration);
                playTestTone(freq, duration);
            } else if (command == "checkgpio") {
                // 检查I2S GPIO状态
                Serial.println("\n========== GPIO状态检查 ==========");
                Serial.printf("I2S扬声器GPIO配置:\n");
                Serial.printf("  BCLK  = GPIO%d\n", I2S_SPK_SCK_PIN);
                Serial.printf("  LRCLK = GPIO%d\n", I2S_SPK_WS_PIN);
                Serial.printf("  DIN   = GPIO%d\n", I2S_SPK_SD_PIN);
                Serial.println("\n⚠️ MAX98357A硬件检查：");
                Serial.println("1. SD引脚必须悬空或接5V (不能接GND!)");
                Serial.println("2. VIN引脚必须接5V (不能接3.3V)");
                Serial.println("3. GAIN引脚悬空可获得最大15dB增益");
                Serial.println("4. 扬声器阻抗应为4-8欧姆");
                Serial.println("\n详细故障排查请查看: MAX98357A_TROUBLESHOOTING.md");
                Serial.println("====================================\n");
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
    if (isRecording && audioBuffer) {
        size_t bytes_read = 0;
        int32_t i2s_buffer[512];  // 32位I2S缓冲区，512样本×4字节=2048字节
        
        esp_err_t result = i2s_read(I2S_NUM_0, i2s_buffer, sizeof(i2s_buffer), &bytes_read, 100);
        
        // 定期输出I2S读取状态
        static unsigned long lastI2SDebug = 0;
        static int i2sReadCount = 0;
        static int i2sSuccessCount = 0;
        i2sReadCount++;
        
        if (millis() - lastI2SDebug > 2000) {
            Serial.printf("[I2S-DEBUG] 读取次数: %d, 成功: %d, 最后结果: %d, 字节: %d\n", 
                          i2sReadCount, i2sSuccessCount, result, bytes_read);
            
            // 输出原始32位数据样本
            if (bytes_read > 0) {
                Serial.print("[I2S-DEBUG] 原始32位样本值: ");
                for (int i = 0; i < 5 && i < bytes_read/4; i++) {
                    Serial.printf("%d(0x%08X) ", i2s_buffer[i], i2s_buffer[i]);
                }
                Serial.println();
            }
            
            lastI2SDebug = millis();
            i2sReadCount = 0;
            i2sSuccessCount = 0;
        }
        
        if (result == ESP_OK && bytes_read > 0) {
            i2sSuccessCount++;
            
            // 转换32位数据到16位
            size_t samples_read = bytes_read / 4;  // 32位 = 4字节
            
            if (useRealtimeASR) {
                // 实时识别模式
                uint8_t pcm_buffer[samples_read * 2];  // 16位 = 2字节
                int16_t* output_ptr = (int16_t*)pcm_buffer;
                
                for (size_t i = 0; i < samples_read; i++) {
                    // INMP441的数据在32位的高18位，右移14位得到16位数据
                    output_ptr[i] = (int16_t)(i2s_buffer[i] >> 14);
                }
                
                // 检查WebSocket是否已连接
                if (realtimeASR.isConnected()) {
                    // WebSocket已连接，直接发送音频数据
                    bool sendResult = realtimeASR.sendAudioData(pcm_buffer, samples_read * 2);
                    
                    // 定期输出调试信息
                    static unsigned long lastDebugTime = 0;
                    static int sendCount = 0;
                    static size_t totalSent = 0;
                    
                    if (sendResult) {
                        sendCount++;
                        totalSent += samples_read * 2;
                    }
                    
                    if (millis() - lastDebugTime > 1000) {
                        Serial.printf("[REALTIME-DEBUG] 已发送 %d 次音频数据，总计 %d bytes\n", 
                                      sendCount, totalSent);
                        
                        // 检查音频电平
                        uint32_t sum = 0;
                        uint16_t maxLevel = 0;
                        for (size_t i = 0; i < samples_read && i < 512; i++) {
                            uint16_t level = abs(output_ptr[i]);
                            sum += level;
                            if (level > maxLevel) maxLevel = level;
                        }
                        if (samples_read > 0) {
                            Serial.printf("[REALTIME-DEBUG] 音频电平 - 平均: %d, 最大: %d\n", 
                                          sum / samples_read, maxLevel);
                        }
                        
                        lastDebugTime = millis();
                        sendCount = 0;
                        totalSent = 0;
                    }
                } else {
                    // WebSocket未连接，先缓存到audioBuffer（等连接后发送）
                    if (recordedSize < audioBufferSize - 2048) {
                        memcpy(audioBuffer + recordedSize, pcm_buffer, samples_read * 2);
                        recordedSize += samples_read * 2;
                        
                        static unsigned long lastBufferInfo = 0;
                        if (millis() - lastBufferInfo > 500) {
                            Serial.printf("[REALTIME] WebSocket连接中，已缓存 %d bytes 音频\n", recordedSize);
                            lastBufferInfo = millis();
                        }
                    }
                }
                
            } else {
                // 传统模式：缓存所有音频
                if (recordedSize < audioBufferSize - 2048) {
                    int16_t* output_ptr = (int16_t*)(audioBuffer + recordedSize);
                    
                    for (size_t i = 0; i < samples_read; i++) {
                        output_ptr[i] = (int16_t)(i2s_buffer[i] >> 14);
                    }
                    
                    recordedSize += samples_read * 2;
                    recordBuffer = audioBuffer; // 指向录音数据
                    
                    // 计算音频电平（检查转换后的16位数据）
                    static unsigned long lastLevelCheck = 0;
                    if (millis() - lastLevelCheck > 500) { // 每500ms检查一次
                        uint32_t sum = 0;
                        uint16_t maxLevel = 0;
                        
                        for (size_t i = 0; i < samples_read; i++) {
                            uint16_t level = abs(output_ptr[i]);
                            sum += level;
                            if (level > maxLevel) maxLevel = level;
                        }
                        
                        if (samples_read > 0) {
                            uint16_t avgLevel = sum / samples_read;
                            Serial.printf("[RECORD] 电平 - 平均: %d, 最大: %d, 已录制: %d bytes\n", 
                                        avgLevel, maxLevel, recordedSize);
                        }
                        lastLevelCheck = millis();
                    }
                }
            }
        }
    }
}