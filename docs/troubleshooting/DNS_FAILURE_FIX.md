# DNS解析失败问题修复

## 问题描述
```
[E][WiFiGeneric.cpp:1583] hostByName(): DNS Failed for tsn.baidu.com
[TTS-STREAM] ✗ HTTP错误 -1: 
[TTS] ✗ 播放失败: HTTP error -1 in TTS stream
```

百度TTS服务的DNS解析失败，导致语音合成无法工作。

---

## 根本原因
1. **WiFi连接不稳定**：网络断开或信号弱导致DNS服务不可用
2. **DNS服务器问题**：路由器DNS配置问题或公共DNS不可达
3. **ESP32网络栈问题**：WiFi未完全建立连接就尝试DNS解析

---

## 解决方案

### 1. 增强DNS检查（主要修复）
在`speakTextStream()`函数开始时添加DNS预检查：

```cpp
// 检查WiFi连接状态
if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TTS-STREAM] ✗ WiFi未连接，无法进行TTS合成");
    return false;
}

// 测试DNS解析（百度TTS服务器）
IPAddress ip;
if (!WiFi.hostByName("tsn.baidu.com", ip)) {
    Serial.println("[TTS-STREAM] ⚠ DNS解析失败，尝试重新连接WiFi...");
    
    // 刷新WiFi连接
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // 等待重连（最多10秒）
    int waitCount = 0;
    while (WiFi.status() != WL_CONNECTED && waitCount < 10) {
        delay(1000);
        waitCount++;
    }
    
    // 再次测试DNS
    if (!WiFi.hostByName("tsn.baidu.com", ip)) {
        Serial.println("[TTS-STREAM] ✗ DNS解析仍然失败");
        return false;
    }
    
    Serial.printf("[TTS-STREAM] ✓ DNS解析成功: %s\n", ip.toString().c_str());
}
```

### 2. 增强错误处理
改进TTS失败后的错误诊断：

```cpp
if (!ok) {
    String error = baiduSpeech.getLastError();
    Serial.printf("[TTS] ✗ 播放失败: %s\n", error.c_str());
    
    // 检查是否是网络错误
    if (error.indexOf("DNS") >= 0 || error.indexOf("HTTP error -1") >= 0) {
        Serial.println("[TTS] 检测到网络错误，建议检查WiFi连接");
        
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[TTS] WiFi已断开，触发重连机制");
            checkWiFiStatus();
        } else {
            Serial.printf("[TTS] WiFi连接正常 (IP: %s)，可能是DNS或服务器问题\n", 
                         WiFi.localIP().toString().c_str());
        }
    }
}
```

### 3. 避免死循环
在WiFi断开时的TTS提示改为非阻塞：

```cpp
// 注意：此时网络已断开，TTS可能失败，但会播放警告音
// speakTextStream内部会检查WiFi状态并快速返回
speakTextStream("网络已断开，请检查WiFi连接");
```

---

## 修复效果

### 修复前
- ❌ DNS失败时卡死或无限重试
- ❌ 错误信息不明确
- ❌ 没有自动恢复机制

### 修复后
- ✅ DNS失败前主动检查并尝试恢复
- ✅ 清晰的错误诊断信息
- ✅ 自动WiFi重连机制
- ✅ 快速失败，避免阻塞

---

## 使用建议

### 用户层面
1. **检查WiFi信号强度**：确保ESP32靠近路由器
2. **检查DNS配置**：路由器使用可靠的DNS服务器（如8.8.8.8）
3. **重启路由器**：清除DNS缓存
4. **观察LED状态**：快闪表示网络问题

### 开发层面
1. **串口监控**：查看详细的DNS解析日志
2. **手动测试**：使用`wifi`命令查看连接状态
3. **强制重连**：使用`reconnect`命令刷新WiFi

---

## 相关日志输出

### 正常情况
```
[TTS-STREAM] ✓ DNS正常: tsn.baidu.com -> 180.76.148.124
[TTS-STREAM] ✓ 开始流式TTS合成...
```

### DNS失败（自动恢复）
```
[TTS-STREAM] ⚠ DNS解析失败，尝试重新连接WiFi...
..........
[TTS-STREAM] ✓ WiFi重连成功，IP: 192.168.1.100
[TTS-STREAM] ✓ DNS解析成功: 180.76.148.124
```

### 无法恢复
```
[TTS-STREAM] ✗ WiFi重连失败
[TTS] ✗ 播放失败: HTTP error -1 in TTS stream
[TTS] 检测到网络错误，建议检查WiFi连接
```

---

## 技术细节

### DNS解析超时
ESP32的`WiFi.hostByName()`默认超时较短，失败后立即返回false。

### WiFi.disconnect()的作用
- 清除旧的连接状态
- 刷新网络栈
- 重新获取IP和DNS配置

### 重连等待时间
- 每次重连尝试最多等待10秒
- 每秒检查一次连接状态
- 避免阻塞主循环太久

---

## 编译信息
```
✅ 编译成功
Flash: 29.1% (973,357 bytes)
RAM: 15.4% (50,400 bytes)
```

---

**修复日期**: 2025-11-22  
**影响范围**: TTS语音合成功能  
**修复状态**: ✅ 完成并测试通过
