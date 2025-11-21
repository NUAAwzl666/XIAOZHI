#include "BaiduSpeech.h"
#include <base64.h>

BaiduSpeech::BaiduSpeech() : tokenExpireTime(0), initialized(false) {
    // 设置API URLs
    tokenUrl = "https://aip.baidubce.com/oauth/2.0/token";
    asrUrl = "https://vop.baidu.com/server_api";
    ttsUrl = "https://tsn.baidu.com/text2audio";
    
    // 默认配置
    asrLanguage = "zh";
    asrSampleRate = 16000;
    ttsVoice = "zh";
    ttsSpeed = 5;
    ttsPitch = 5;
    ttsVolume = 5;
}

BaiduSpeech::~BaiduSpeech() {
    // 不再需要停止持久连接
}

bool BaiduSpeech::begin(const String& appId, const String& apiKey, const String& secretKey) {
    this->appId = appId;
    this->apiKey = apiKey;
    this->secretKey = secretKey;
    
    bool success = getAccessToken();
    if (success) {
        initialized = true;
        lastError = "";
        Serial.println("[BAIDU] 百度语音服务初始化成功");
    } else {
        initialized = false;
        lastError = "Failed to get access token";
        Serial.println("[BAIDU] 百度语音服务初始化失败");
    }
    
    return success;
}

bool BaiduSpeech::getAccessToken() {
    Serial.println("[BAIDU] 开始获取访问令牌...");
    Serial.printf("[BAIDU] Token URL: %s\n", tokenUrl.c_str());
    
    WiFiClientSecure tempClient;  // 创建临时client
    tempClient.setInsecure();
    tempClient.setTimeout(15000);
    
    Serial.println("[BAIDU] 创建HTTP客户端...");
    HTTPClient http;
    http.begin(tempClient, tokenUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.setTimeout(15000);
    
    String postData = "grant_type=client_credentials";
    postData += "&client_id=" + apiKey;
    postData += "&client_secret=" + secretKey;
    
    Serial.printf("[BAIDU] 发送POST请求，数据长度: %d bytes\n", postData.length());
    Serial.printf("[BAIDU] 可用内存: %d bytes\n", ESP.getFreeHeap());
    
    int httpCode = http.POST(postData);
    
    Serial.printf("[BAIDU] HTTP响应码: %d\n", httpCode);
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.printf("[BAIDU] 响应长度: %d bytes\n", response.length());
        
        if (response.length() > 0) {
            String preview = response.substring(0, min(200, (int)response.length()));
            Serial.printf("[BAIDU] 响应预览: %s\n", preview.c_str());
        }
        
        DynamicJsonDocument doc(2048);  // 增加缓冲区大小到2048字节
        
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            if (doc.containsKey("access_token")) {
                accessToken = doc["access_token"].as<String>();
                tokenExpireTime = millis() + (doc["expires_in"].as<unsigned long>() * 1000);
                Serial.printf("[BAIDU] ✓ 成功获取访问令牌，有效期: %lu 秒\n", doc["expires_in"].as<unsigned long>());
                http.end();
                return true;
            } else {
                lastError = "Token response missing access_token";
                if (doc.containsKey("error_description")) {
                    lastError += ": " + doc["error_description"].as<String>();
                }
                Serial.printf("[BAIDU] JSON错误: %s\n", lastError.c_str());
            }
        } else {
            lastError = "Failed to parse token response JSON";
            Serial.printf("[BAIDU] JSON解析失败，原始响应: %s\n", response.c_str());
        }
    } else {
        lastError = "HTTP error " + String(httpCode) + " getting token";
        String errorResponse = http.getString();
        Serial.printf("[BAIDU] HTTP错误 %d: %s\n", httpCode, errorResponse.c_str());
        
        if (httpCode == -1) {
            Serial.println("[BAIDU] 网络连接失败");
        } else if (httpCode == 401) {
            Serial.println("[BAIDU] 认证失败，请检查API密钥");
        } else if (httpCode == 403) {
            Serial.println("[BAIDU] 访问被拒绝");
        }
    }
    
    http.end();
    return false;
}

bool BaiduSpeech::isTokenValid() {
    return (millis() < tokenExpireTime) && (accessToken.length() > 0);
}

String BaiduSpeech::base64Encode(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        Serial.println("[BAIDU-ASR] ✗ Base64编码：空数据");
        return "";
    }
    
    Serial.printf("[BAIDU-ASR] Base64编码：输入数据大小 %d bytes\n", length);
    
    // 直接使用自定义编码方法，避免第三方库的内存问题
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    // 预先计算结果长度并预分配
    size_t encodedLength = ((length + 2) / 3) * 4;
    Serial.printf("[BAIDU-ASR] 预计Base64长度: %d bytes\n", encodedLength);
    
    // 检查内存
    if (ESP.getFreeHeap() < encodedLength + 5000) {
        Serial.printf("[BAIDU-ASR] ✗ 内存不足，无法编码: 需要%d, 可用%d\n", encodedLength + 5000, ESP.getFreeHeap());
        return "";
    }
    
    String result;
    result.reserve(encodedLength);  // 预分配空间
    
    size_t i = 0;
    while (i < length) {
        uint32_t octet_a = i < length ? data[i++] : 0;
        uint32_t octet_b = i < length ? data[i++] : 0;
        uint32_t octet_c = i < length ? data[i++] : 0;
        
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        result += chars[(triple >> 3 * 6) & 0x3F];
        result += chars[(triple >> 2 * 6) & 0x3F];
        result += chars[(triple >> 1 * 6) & 0x3F];
        result += chars[(triple >> 0 * 6) & 0x3F];
    }
    
    // 添加填充
    int mod = length % 3;
    if (mod == 1) {
        result = result.substring(0, result.length() - 2) + "==";
    } else if (mod == 2) {
        result = result.substring(0, result.length() - 1) + "=";
    }
    
    Serial.printf("[BAIDU-ASR] Base64编码完成，长度: %d\n", result.length());
    return result;
}

// 对TTS文本进行URL编码（UTF-8按字节百分号编码）
String BaiduSpeech::urlEncode(const String& value) {
    String out;
    out.reserve(value.length() * 3);
    auto isUnreserved = [](char c) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) return true;
        switch (c) { case '-': case '_': case '.': case '~': return true; }
        return false;
    };
    const char* p = value.c_str();
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (isUnreserved((char)c)) {
            out += (char)c;
        } else if (c == ' ') {
            out += "%20";
        } else {
            static const char* hex = "0123456789ABCDEF";
            out += '%';
            out += hex[(c >> 4) & 0x0F];
            out += hex[c & 0x0F];
        }
        ++p;
    }
    return out;
}

// 过滤不被TTS支持的字符，如emoji（大多为4字节UTF-8），并规范换行。
String BaiduSpeech::sanitizeTTS(const String& text) {
    String out;
    out.reserve(text.length());
    const uint8_t* s = (const uint8_t*)text.c_str();
    size_t i = 0, n = text.length();
    while (i < n) {
        uint8_t b = s[i];
        if (b < 0x80) { // 1字节ASCII
            char c = (char)b;
            if (c == '\n' || c == '\r') {
                out += "。";
            } else {
                out += c;
            }
            i += 1;
        } else if ((b & 0xE0) == 0xC0 && i + 1 < n) { // 2字节
            out += (char)s[i];
            out += (char)s[i + 1];
            i += 2;
        } else if ((b & 0xF0) == 0xE0 && i + 2 < n) { // 3字节（常见中文）
            out += (char)s[i];
            out += (char)s[i + 1];
            out += (char)s[i + 2];
            i += 3;
        } else if ((b & 0xF8) == 0xF0 && i + 3 < n) { // 4字节（多为emoji）
            i += 4; // 跳过
        } else {
            i += 1; // 异常字节，跳过
        }
        // 粗略限制，避免超长文本导致TTS报参错
        if (out.length() > 600) {
            out += "…";
            break;
        }
    }
    return out;
}

String BaiduSpeech::recognizeSpeech(const uint8_t* audioData, size_t dataSize, const String& format) {
    if (!initialized || !isTokenValid()) {
        if (!getAccessToken()) {
            lastError = "Failed to refresh access token";
            return "";
        }
    }
    
    Serial.printf("[BAIDU-ASR] 开始语音识别，音频大小: %d bytes\n", dataSize);
    Serial.printf("[BAIDU-ASR] 可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    // 检查内存是否足够
    size_t estimatedMemoryNeeded = dataSize * 2; // Base64大约是原数据的1.33倍，再加上JSON开销
    if (ESP.getFreeHeap() < estimatedMemoryNeeded) {
        lastError = "Insufficient memory for Base64 encoding: need " + String(estimatedMemoryNeeded) + ", have " + String(ESP.getFreeHeap());
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    // 检查音频数据有效性
    if (dataSize < 1000) {
        lastError = "Audio data too small: " + String(dataSize) + " bytes";
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    if (audioData == nullptr) {
        lastError = "Audio data is null";
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    // 检查音频数据内容（更全面的验证）
    bool hasContent = false;
    size_t nonZeroCount = 0;
    size_t checkSize = min((size_t)5000, dataSize); // 检查前5000字节
    
    Serial.printf("[BAIDU-ASR] 检查音频数据内容，检查范围: %d bytes\n", checkSize);
    
    for (size_t i = 0; i < checkSize; i++) {
        if (audioData[i] != 0) {
            hasContent = true;
            nonZeroCount++;
        }
    }
    
    Serial.printf("[BAIDU-ASR] 前%d字节中找到%d个非零字节\n", checkSize, nonZeroCount);
    
    // 输出前20个字节用于调试
    Serial.print("[BAIDU-ASR] 前20个字节: ");
    for (size_t i = 0; i < min((size_t)20, dataSize); i++) {
        Serial.printf("%02X ", audioData[i]);
    }
    Serial.println();
    
    // 检查中间部分
    if (dataSize > 2000) {
        size_t midStart = dataSize / 2 - 10;
        Serial.print("[BAIDU-ASR] 中间20个字节: ");
        for (size_t i = 0; i < 20 && (midStart + i) < dataSize; i++) {
            Serial.printf("%02X ", audioData[midStart + i]);
        }
        Serial.println();
    }
    
    if (!hasContent) {
        lastError = "Audio data appears to be empty (all zeros)";
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    Serial.printf("[BAIDU-ASR] 开始前可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    WiFiClientSecure tempClient;  // 创建临时client
    tempClient.setInsecure();
    tempClient.setTimeout(30000);  // 增加超时时间到30秒
    
    HTTPClient http;
    String url = asrUrl;
    http.begin(tempClient, url);
    http.setTimeout(30000);
    
    // 设置请求头
    http.addHeader("Content-Type", "application/json");
    
    // 构建请求体
    Serial.println("[BAIDU-ASR] 开始Base64编码...");
    String base64Audio = base64Encode(audioData, dataSize);
    Serial.printf("[BAIDU-ASR] Base64编码完成，长度: %d\n", base64Audio.length());
    Serial.printf("[BAIDU-ASR] Base64编码后可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    if (base64Audio.length() == 0) {
        lastError = "Base64 encoding failed";
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    // 计算需要的JSON文档大小（Base64长度 + 其他字段的开销）
    size_t jsonSize = base64Audio.length() + 2048;  // 增加额外空间到2048字节
    Serial.printf("[BAIDU-ASR] 分配JSON文档大小: %d bytes\n", jsonSize);
    
    // 检查是否有足够内存分配JSON文档
    if (ESP.getFreeHeap() < jsonSize + 5000) { // 保留5KB缓冲
        lastError = "Insufficient memory for JSON document: need " + String(jsonSize + 5000) + ", have " + String(ESP.getFreeHeap());
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    DynamicJsonDocument doc(jsonSize);
    
    // 先添加其他字段
    doc["format"] = format;
    doc["rate"] = asrSampleRate;
    doc["channel"] = 1;
    doc["cuid"] = "esp32_s3_voice_assistant";
    doc["token"] = accessToken;
    doc["len"] = dataSize;
    doc["lan"] = "zh";  // 明确指定中文
    doc["dev_pid"] = 1536;  // 普通话(支持简单的英文识别)
    
    Serial.printf("[BAIDU-ASR] JSON文档分配后可用内存: %d bytes\n", ESP.getFreeHeap());
    
    // 最后添加大的speech字段 - 使用移动语义避免复制
    doc["speech"] = base64Audio;
    
    // 检查speech字段是否成功添加
    if (doc["speech"].isNull() || doc["speech"].as<String>().length() == 0) {
        lastError = "Failed to add speech field to JSON - memory overflow";
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        Serial.printf("[BAIDU-ASR] ✗ JSON文档容量: %d, 已使用: %d\n", doc.capacity(), doc.memoryUsage());
        http.end();
        return "";
    }
    
    Serial.printf("[BAIDU-ASR] 请求参数: format=%s, rate=%d, channel=1, len=%d, lan=zh, dev_pid=1536\n", 
                  format.c_str(), asrSampleRate, dataSize);
    Serial.printf("[BAIDU-ASR] speech字段长度: %d bytes\n", doc["speech"].as<String>().length());
    
    String requestBody;
    size_t jsonLength = serializeJson(doc, requestBody);
    
    if (jsonLength == 0 || requestBody.length() < 100) {
        lastError = "JSON serialization failed - returned " + String(jsonLength) + " bytes, requestBody length: " + String(requestBody.length());
        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
        Serial.printf("[BAIDU-ASR] ✗ JSON文档容量: %d, 已使用: %d\n", doc.capacity(), doc.memoryUsage());
        Serial.printf("[BAIDU-ASR] ✗ Base64长度: %d\n", base64Audio.length());
        
        // 输出前100个字符查看问题
        if (requestBody.length() > 0) {
            Serial.printf("[BAIDU-ASR] ✗ 请求体内容: %s\n", requestBody.substring(0, min(100, (int)requestBody.length())).c_str());
        }
        
        http.end();
        return "";
    }
    
    Serial.printf("[BAIDU-ASR] JSON序列化成功，负载大小: %d bytes\n", requestBody.length());
    Serial.printf("[BAIDU-ASR] JSON文档使用内存: %d / %d bytes\n", doc.memoryUsage(), doc.capacity());
    Serial.printf("[BAIDU-ASR] 检查JSON中的speech字段长度: %s\n", doc["speech"].as<String>().length() > 0 ? "有数据" : "空数据");
    
    // 释放JSON文档和Base64字符串以腾出内存给SSL
    base64Audio = "";  // 清空base64字符串
    doc.clear();       // 清空JSON文档
    Serial.printf("[BAIDU-ASR] 清理后可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    Serial.printf("[BAIDU-ASR] 发送识别请求，负载大小: %d bytes\n", requestBody.length());
    
    int httpCode = http.POST(requestBody);
    Serial.printf("[BAIDU-ASR] HTTP响应码: %d\n", httpCode);
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.printf("[BAIDU-ASR] 响应长度: %d bytes\n", response.length());
        
        // 输出响应的前200个字符用于调试
        String preview = response.substring(0, min(200, (int)response.length()));
        Serial.printf("[BAIDU-ASR] 响应预览: %s\n", preview.c_str());
        
        DynamicJsonDocument responseDoc(2048);
        
        if (deserializeJson(responseDoc, response) == DeserializationError::Ok) {
            int errNo = responseDoc["err_no"].as<int>();
            Serial.printf("[BAIDU-ASR] 错误码: %d\n", errNo);
            
            if (errNo == 0) {
                if (responseDoc.containsKey("result")) {
                    JsonArray results = responseDoc["result"];
                    Serial.printf("[BAIDU-ASR] 结果数组大小: %d\n", results.size());
                    if (results.size() > 0) {
                        String recognizedText = results[0].as<String>();
                        Serial.printf("[BAIDU-ASR] ✓ 识别成功: '%s' (长度: %d)\n", recognizedText.c_str(), recognizedText.length());
                        
                        // 检查是否为空结果
                        if (recognizedText.length() == 0) {
                            Serial.println("[BAIDU-ASR] ⚠️ 识别结果为空，可能是音频质量问题");
                            lastError = "Empty recognition result - audio quality may be poor";
                            http.end();
                            return "";
                        }
                        
                        http.end();
                        return recognizedText;
                    } else {
                        lastError = "ASR error: empty result array";
                        Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
                    }
                } else {
                    lastError = "ASR error: no result field in response";
                    Serial.printf("[BAIDU-ASR] ✗ %s\n", lastError.c_str());
                }
            } else {
                String errMsg = responseDoc["err_msg"].as<String>();
                lastError = "ASR error: " + errMsg;
                Serial.printf("[BAIDU-ASR] ✗ 错误 %d: %s\n", errNo, errMsg.c_str());
                
                // 针对常见错误提供建议
                if (errNo == 3307) {
                    Serial.println("[BAIDU-ASR] 💡 错误3307建议：");
                    Serial.println("    1. 音频数据可能太短（需要至少1秒）");
                    Serial.println("    2. 音频格式可能不正确");
                    Serial.println("    3. 音频数据可能为空或损坏");
                    Serial.println("    4. 尝试录制更长的音频（2-3秒）");
                } else if (errNo == 3301) {
                    Serial.println("[BAIDU-ASR] 💡 错误3301建议：检查音频编码格式");
                } else if (errNo == 3302) {
                    Serial.println("[BAIDU-ASR] 💡 错误3302建议：检查采样率设置");
                }
            }
        } else {
            lastError = "Failed to parse ASR response JSON";
            Serial.printf("[BAIDU-ASR] ✗ JSON解析失败，原始响应: %s\n", response.c_str());
        }
    } else {
        String errorResponse = http.getString();
        lastError = "HTTP error " + String(httpCode) + " in ASR";
        Serial.printf("[BAIDU-ASR] ✗ HTTP错误 %d: %s\n", httpCode, errorResponse.c_str());
    }
    
    http.end();
    return "";
}

// RAW格式上传 - 直接发送PCM数据，无需Base64编码，大幅节省内存
String BaiduSpeech::recognizeSpeechRaw(const uint8_t* audioData, size_t dataSize, int sampleRate) {
    if (!initialized || !isTokenValid()) {
        if (!getAccessToken()) {
            lastError = "Failed to refresh access token";
            return "";
        }
    }
    
    Serial.printf("[BAIDU-ASR-RAW] 开始RAW格式语音识别，音频大小: %d bytes\n", dataSize);
    Serial.printf("[BAIDU-ASR-RAW] 可用内存: %d bytes (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    // 检查音频数据
    if (dataSize < 1000 || audioData == nullptr) {
        lastError = "Audio data too small or null";
        Serial.printf("[BAIDU-ASR-RAW] ✗ %s\n", lastError.c_str());
        return "";
    }
    
    // 构建URL参数
    String url = asrUrl;
    url += "?cuid=esp32_s3_voice_assistant";
    url += "&token=" + accessToken;
    url += "&dev_pid=1537";  // 普通话
    url += "&lan=zh";
    
    Serial.printf("[BAIDU-ASR-RAW] 请求URL: %s\n", url.c_str());
    
    WiFiClientSecure tempClient;
    tempClient.setInsecure();
    tempClient.setTimeout(30000);
    
    HTTPClient http;
    http.begin(tempClient, url);
    http.setTimeout(30000);
    
    // 设置Content-Type为RAW格式
    String contentType = "audio/pcm;rate=" + String(sampleRate);
    http.addHeader("Content-Type", contentType);
    
    Serial.printf("[BAIDU-ASR-RAW] Content-Type: %s\n", contentType.c_str());
    Serial.printf("[BAIDU-ASR-RAW] 发送请求...POST前可用内存: %d bytes\n", ESP.getFreeHeap());
    
    // 直接POST二进制数据
    int httpCode = http.POST((uint8_t*)audioData, dataSize);
    
    Serial.printf("[BAIDU-ASR-RAW] HTTP响应码: %d\n", httpCode);
    
    if (httpCode == 200) {
        String response = http.getString();
        Serial.printf("[BAIDU-ASR-RAW] 响应长度: %d bytes\n", response.length());
        Serial.printf("[BAIDU-ASR-RAW] 响应: %s\n", response.c_str());
        
        DynamicJsonDocument responseDoc(2048);
        
        if (deserializeJson(responseDoc, response) == DeserializationError::Ok) {
            int errNo = responseDoc["err_no"].as<int>();
            
            if (errNo == 0) {
                // 识别成功
                if (responseDoc.containsKey("result") && responseDoc["result"].size() > 0) {
                    String result = responseDoc["result"][0].as<String>();
                    Serial.printf("[BAIDU-ASR-RAW] ✓ 识别成功: %s\n", result.c_str());
                    http.end();
                    return result;
                }
            } else {
                // 识别失败
                String errMsg = responseDoc["err_msg"].as<String>();
                lastError = "ASR error: " + errMsg;
                Serial.printf("[BAIDU-ASR-RAW] ✗ 错误 %d: %s\n", errNo, errMsg.c_str());
            }
        } else {
            lastError = "Failed to parse ASR response JSON";
            Serial.printf("[BAIDU-ASR-RAW] ✗ JSON解析失败\n");
        }
    } else {
        String errorResponse = http.getString();
        lastError = "HTTP error " + String(httpCode);
        Serial.printf("[BAIDU-ASR-RAW] ✗ HTTP错误 %d: %s\n", httpCode, errorResponse.c_str());
    }
    
    http.end();
    return "";
}

bool BaiduSpeech::synthesizeSpeech(const String& text, uint8_t** audioData, size_t* dataSize) {
    if (!initialized || !isTokenValid()) {
        if (!getAccessToken()) {
            lastError = "Failed to refresh access token";
            return false;
        }
    }
    
    WiFiClientSecure tempClient;  // 创建临时client
    tempClient.setInsecure();
    
    HTTPClient http;
    http.begin(tempClient, ttsUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // 构建POST数据（先清洗再URL编码）
    String cleanText = sanitizeTTS(text);
    String encodedTex = urlEncode(cleanText);
    if (encodedTex.length() > 1024) {
        encodedTex = encodedTex.substring(0, 1024);
    }

    String postData = "tex=" + encodedTex;
    postData += "&tok=" + accessToken;
    postData += "&cuid=esp32_device";
    postData += "&ctp=1";
    postData += "&lan=" + ttsVoice;
    postData += "&spd=" + String(ttsSpeed);
    postData += "&pit=" + String(ttsPitch);
    postData += "&vol=" + String(ttsVolume);
    postData += "&per=0";
    postData += "&aue=6"; // 6=wav，便于直接I2S播放
    
    int httpCode = http.POST(postData);
    
    if (httpCode == 200) {
        String contentType = http.header("Content-Type");
        
        if (contentType.indexOf("audio") >= 0) {
            // 获取音频数据
            int contentLength = http.getSize();
            if (contentLength > 0) {
                *audioData = (uint8_t*)malloc(contentLength);
                if (*audioData) {
                    WiFiClient* stream = http.getStreamPtr();
                    size_t bytesRead = stream->readBytes(*audioData, contentLength);
                    *dataSize = bytesRead;
                    http.end();
                    return true;
                } else {
                    lastError = "Failed to allocate memory for audio data";
                }
            } else {
                lastError = "Empty audio response";
            }
        } else {
            // 可能是错误响应（JSON格式）
            String response = http.getString();
            DynamicJsonDocument doc(1024);
            if (deserializeJson(doc, response) == DeserializationError::Ok) {
                lastError = "TTS error: " + doc["err_msg"].as<String>();
            } else {
                lastError = "Unknown TTS error";
            }
        }
    } else {
        lastError = "HTTP error " + String(httpCode) + " in TTS";
    }
    
    http.end();
    return false;
}

void BaiduSpeech::setASRConfig(const String& language, int sampleRate) {
    asrLanguage = language;
    asrSampleRate = sampleRate;
}

void BaiduSpeech::setTTSConfig(const String& voice, int speed, int pitch, int volume) {
    ttsVoice = voice;
    ttsSpeed = speed;
    ttsPitch = pitch;
    ttsVolume = volume;
}

bool BaiduSpeech::isInitialized() const {
    return initialized;
}

String BaiduSpeech::getLastError() const {
    return lastError;
}

String BaiduSpeech::recognizeSpeechFromFile(const String& filePath) {
    // 这个方法需要实现文件读取，暂时返回空字符串
    lastError = "File recognition not implemented yet";
    return "";
}

bool BaiduSpeech::synthesizeSpeechToFile(const String& text, const String& filePath) {
    // 这个方法需要实现文件写入，暂时返回false
    lastError = "File synthesis not implemented yet";
    return false;
}

// 流式语音合成：以PCM/WAV形式流式读取音频数据并通过回调返回
bool BaiduSpeech::synthesizeSpeechStream(const String& text,
                                std::function<bool(const uint8_t* chunk, size_t len)> onAudioChunk,
                                int sampleRate,
                                int aue) {
    if (!initialized || !isTokenValid()) {
        if (!getAccessToken()) {
            lastError = "Failed to refresh access token";
            return false;
        }
    }

    if (!onAudioChunk) {
        lastError = "onAudioChunk callback is null";
        return false;
    }

    WiFiClientSecure tempClient;
    tempClient.setInsecure();
    tempClient.setTimeout(20000);

    HTTPClient http;
    http.begin(tempClient, ttsUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // 文本清洗与编码后再请求，避免接口报参错
    String cleanText = sanitizeTTS(text);
    String encodedTex = urlEncode(cleanText);
    if (encodedTex.length() > 1024) {
        encodedTex = encodedTex.substring(0, 1024);
    }

    // Serial.printf("[TTS-STREAM] 原始文本: %s\n", text.c_str());
    // Serial.printf("[TTS-STREAM] 清洗后: %s\n", cleanText.c_str());
    // Serial.printf("[TTS-STREAM] URL编码长度: %d\n", encodedTex.length());

    // 使用WAV/PCM（aue=6/4）方便直接I2S播放
    String postData = "tex=" + encodedTex;
    postData += "&tok=" + accessToken;
    postData += "&cuid=esp32_device";
    postData += "&ctp=1";
    postData += "&lan=" + ttsVoice;
    postData += "&spd=" + String(ttsSpeed);
    postData += "&pit=" + String(ttsPitch);
    postData += "&vol=" + String(ttsVolume);
    postData += "&per=0"; // 发音人，默认
    postData += "&aue=" + String(aue); // 6=wav，4=pcm-16k，5=pcm-8k

    // Serial.printf("[TTS-STREAM] POST数据长度: %d bytes\n", postData.length());
    // Serial.printf("[TTS-STREAM] 发送POST请求到: %s\n", ttsUrl.c_str());
    // Serial.printf("[TTS-STREAM] 可用内存: %d bytes\n", ESP.getFreeHeap());
    
    int httpCode = http.POST(postData);
    // Serial.printf("[TTS-STREAM] HTTP响应码: %d\n", httpCode);
    
    if (httpCode != 200) {
        String errorBody = http.getString();
        lastError = "HTTP error " + String(httpCode) + " in TTS stream";
        Serial.printf("[TTS-STREAM] ✗ HTTP错误 %d: %s\n", httpCode, errorBody.c_str());
        http.end();
        return false;
    }

    // 获取流指针并检查前几个字节判断是音频还是JSON
    WiFiClient* stream = http.getStreamPtr();
    
    // 等待数据可用
    unsigned long waitStart = millis();
    while (!stream->available() && stream->connected() && (millis() - waitStart < 5000)) {
        delay(10);
    }
    
    if (!stream->available()) {
        lastError = "No data received from TTS server";
        Serial.println("[TTS-STREAM] ✗ 服务器无响应数据");
        http.end();
        return false;
    }
    
    // 读取前几个字节判断内容类型（读取后需要作为音频数据的一部分）
    uint8_t firstChunk[128];
    int firstLen = 0;
    
    // 尝试读取前128字节
    size_t available = stream->available();
    size_t toRead = available > sizeof(firstChunk) ? sizeof(firstChunk) : available;
    if (toRead > 0) {
        firstLen = stream->readBytes(firstChunk, toRead);
    }
    
    Serial.printf("[TTS-STREAM] 读取前%d字节: ", firstLen);
    for (int i = 0; i < min(20, firstLen); i++) {
        Serial.printf("%02X ", firstChunk[i]);
    }
    Serial.println();
    
    // 检查是否为JSON错误响应（以 '{' 开头）
    if (firstLen > 0 && firstChunk[0] == '{') {
        // 这是JSON错误响应，读取完整响应
        String response = String((char*)firstChunk).substring(0, firstLen);
        response += http.getString(); // 读取剩余部分
        
        Serial.printf("[TTS-STREAM] JSON错误响应: %s\n", response.c_str());
        
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, response) == DeserializationError::Ok) {
            int errNo = doc["err_no"].as<int>();
            String errMsg = doc["err_msg"].as<String>();
            lastError = "TTS error " + String(errNo) + ": " + errMsg;
            Serial.printf("[TTS-STREAM] ✗ 错误码 %d: %s\n", errNo, errMsg.c_str());
        } else {
            lastError = "Unknown TTS JSON error: " + response;
        }
        http.end();
        return false;
    }
    
    // 检查是否为WAV文件（RIFF头）
    bool isWav = false;
    if (firstLen >= 4 && memcmp(firstChunk, "RIFF", 4) == 0) {
        Serial.println("[TTS-STREAM] ✓ 检测到WAV格式");
        isWav = true;
    } else if (firstLen > 0 && firstChunk[0] != '<') {
        // 不是JSON也不是XML，可能是原始PCM
        Serial.println("[TTS-STREAM] ✓ 检测到PCM格式");
    } else {
        lastError = "Unknown response format from TTS";
        Serial.println("[TTS-STREAM] ✗ 未知的响应格式");
        http.end();
        return false;
    }

    const size_t bufSize = 1024;
    uint8_t buf[bufSize];
    bool ok = true;
    
    // 先处理已读取的第一块数据
    if (firstLen > 0) {
        if (!onAudioChunk(firstChunk, firstLen)) {
            http.end();
            return false;
        }
    }

    // 持续读取剩余数据直到服务器关闭连接
    unsigned long lastDataTime = millis();
    unsigned long totalBytes = firstLen;
    int chunkCount = 0;
    
    while (stream->connected() || stream->available()) {
        available = stream->available();
        if (available == 0) {
            // 检查超时：如果5秒内没有新数据，认为传输结束
            if (millis() - lastDataTime > 5000) {
                Serial.println("[TTS-STREAM] 5秒无数据，传输结束");
                break;
            }
            delay(1);
            continue;
        }
        
        lastDataTime = millis();
        toRead = available > bufSize ? bufSize : available;
        int n = stream->readBytes(buf, toRead);
        if (n <= 0) {
            break;
        }
        
        chunkCount++;
        totalBytes += n;
        
        if (!onAudioChunk(buf, (size_t)n)) {
            ok = false;
            break;
        }
    }

    Serial.printf("[TTS-STREAM] 传输完成: 共 %d chunks, %lu bytes\n", chunkCount, totalBytes);
    http.end();
    return ok;
}