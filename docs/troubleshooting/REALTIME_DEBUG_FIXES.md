# 实时识别问题诊断和修复

## 🐛 发现的问题

### 1. 录音时间计算错误
现象: `4294966719 毫秒` (unsigned long下溢)
原因: `recordingStartTime` 在某处被错误设置
修复: 在录音超时检查中调用正确的停止函数

### 2. 音频数据未发送
错误: `-3101: wait audio over time`
含义: 百度服务器等待音频超时（5秒内没收到数据）
原因: 音频数据没有正确发送到WebSocket

### 3. WebSocket自动重连问题
现象: 连接断开后立即重连
原因: `setReconnectInterval(5000)` 导致意外重连
修复: 改为 `setReconnectInterval(0)` 禁用自动重连

## 🔧 已应用的修复

### 修复1: 录音超时处理
```cpp
// 修改前
if (recordingDuration > MAX_RECORD_TIME) {
    stopVoiceRecording();  // 错误：总是调用传统模式
}

// 修改后
if (recordingDuration > MAX_RECORD_TIME) {
    if (useRealtimeASR) {
        stopRealtimeRecording();  // 实时模式
    } else {
        stopVoiceRecording();     // 传统模式
    }
}
```

### 修复2: 添加音频发送调试
```cpp
// 在sendAudioData中添加
static unsigned long totalSent = 0;
static unsigned long lastReportTime = 0;
totalSent += length;

if (millis() - lastReportTime > 2000) {
    Serial.printf("[REALTIME-ASR] 已发送 %lu bytes 音频数据\n", totalSent);
    lastReportTime = millis();
}
```

### 修复3: 添加loop中的调试
```cpp
// 定期输出调试信息
static unsigned long lastDebugTime = 0;
static int sendCount = 0;
sendCount++;

if (millis() - lastDebugTime > 1000) {
    Serial.printf("[REALTIME-DEBUG] 已发送 %d 次音频数据，总计 %d bytes\n", 
                  sendCount, recordedSize);
    
    // 检查音频电平
    Serial.printf("[REALTIME-DEBUG] 音频电平 - 平均: %d, 最大: %d\n", 
                  avgLevel, maxLevel);
    
    lastDebugTime = millis();
    sendCount = 0;
}
```

### 修复4: 禁用WebSocket自动重连
```cpp
// 修改前
webSocket.setReconnectInterval(5000);

// 修改后
webSocket.setReconnectInterval(0);  // 禁用自动重连
```

## 📊 预期的新日志输出

### 正常工作时应该看到：

```
[BUTTON] *** 按钮被按下 - 开始录音 ***
[REALTIME] 开始实时录音...
[REALTIME-ASR] 正在连接WebSocket服务器...
[REALTIME-ASR] Session ID: xxx-xxx-xxx
[REALTIME-ASR] WebSocket已连接
[REALTIME-ASR] ✓ 开始帧发送成功
[实时识别] WebSocket已连接，可以开始说话
[REALTIME] ✓ 开始实时识别，请说话...

# 开始说话后，每秒应该看到：
[REALTIME-DEBUG] 已发送 30 次音频数据，总计 30720 bytes
[REALTIME-DEBUG] 音频电平 - 平均: 245, 最大: 8567

# 每2秒看到：
[REALTIME-ASR] 已发送 61440 bytes 音频数据

# 识别结果：
[实时] 你好
[实时] 你好今天
[最终] 你好今天天气怎么样？ [120-3450 ms]

# 松开按钮：
[BUTTON] *** 按钮被松开 - 停止录音 ***
[REALTIME] 停止实时录音...
[REALTIME] 录音时长: 3500 ms
[REALTIME] 完整识别结果: 你好今天天气怎么样？
[AI] 正在生成回复...
```

## 🔍 诊断检查清单

上传新固件后，请检查：

### ✅ 必须看到的日志
- [ ] `[REALTIME-DEBUG] 已发送 X 次音频数据` - 证明音频在发送
- [ ] `[REALTIME-ASR] 已发送 X bytes 音频数据` - 证明WebSocket发送成功
- [ ] `[REALTIME-DEBUG] 音频电平 - 平均: X, 最大: X` - 证明音频有数据

### ❌ 不应该看到的错误
- [ ] `-3101: wait audio over time` - 如果还有，说明音频未发送
- [ ] `4294966719 毫秒` - 如果还有，说明时间计算仍有问题
- [ ] `未连接，无法发送音频数据` - 如果频繁出现，说明连接不稳定

### 🎯 关键指标
- 音频发送频率: 应该每秒30次左右（512样本×2字节×30 ≈ 30KB/s）
- 音频电平: 说话时应该>100，安静时<50
- 发送总量: 2秒录音应该发送~64KB数据

## 🚀 下一步测试步骤

### 1. 上传固件
```bash
pio run -t upload
```

### 2. 打开串口监视器
```bash
pio device monitor
```

### 3. 测试录音
- 按下按钮
- 等待看到 `[REALTIME] ✓ 开始实时识别，请说话...`
- 立即开始说话："你好"
- 持续1-2秒
- 观察是否有 `[REALTIME-DEBUG]` 输出

### 4. 检查输出

如果看到调试信息 → 音频正在发送，继续说话  
如果没有调试信息 → 音频未录制，检查：
- I2S是否初始化成功
- 麦克风是否正确连接
- `isRecording` 是否为true

### 5. 松开按钮
- 检查是否有识别结果
- 如果有结果 → 成功！
- 如果仍然 `-3101` → 查看发送的总字节数

## 💡 可能的根本原因

### 原因1: I2S读取失败
症状: 没有 `[REALTIME-DEBUG]` 输出  
检查: I2S驱动是否正确安装  
解决: 重新初始化I2S

### 原因2: WebSocket断开
症状: `未连接，无法发送音频数据`  
检查: connected 标志  
解决: 检查网络稳定性

### 原因3: 音频缓冲区问题
症状: 有调试输出但发送字节数为0  
检查: `samples_read` 的值  
解决: 增加I2S读取timeout

### 原因4: 数据转换错误
症状: 有发送但音频电平全为0  
检查: `>>14` 是否正确  
解决: 验证INMP441数据格式

## 📝 临时切换到RAW模式测试

如果实时模式仍有问题，可以临时切换到RAW模式验证硬件：

```cpp
// 在 main.cpp 中找到这一行
bool useRealtimeASR = true;

// 改为
bool useRealtimeASR = false;
```

然后重新编译上传。RAW模式更简单，容易排查硬件问题。

## 🎯 预期结果

修复后应该能看到：
1. 音频持续发送（每秒30次）
2. 发送总量符合预期（2秒≈64KB）
3. 音频电平正常（>100）
4. 收到识别结果
5. 不再出现 -3101 错误

如果还有问题，请提供完整的串口日志，我会进一步分析！
