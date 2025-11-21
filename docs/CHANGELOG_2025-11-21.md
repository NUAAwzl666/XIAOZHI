# 更新日志 - 2025年11月21日

## 重大更新

### 🎯 实时语音识别优化
- **删除非实时模式**：移除传统录音识别，仅保留实时WebSocket流式识别
- **内存优化**：音频缓冲区从64KB降至4KB，节省60KB内存
- **边录边发**：音频数据实时流式发送，不缓存完整录音
- **降低延迟**：WebSocket连接后立即开始录音，减少数据丢失

### 🔧 音频系统改进
- **TTS噪音修复**：
  - 实现奇数字节缓冲处理，确保16-bit音频2字节对齐
  - 降低增益到1.5倍，避免失真
  - 优化DMA缓冲区清理
- **录音数据保护**：
  - 减少WebSocket连接延迟（从50ms到<40ms）
  - 快速跳过I2S噪音数据（2次×20ms）
  - 保护录音开头数据完整性

### 📶 WiFi管理优化
- **快速检测**：WiFi断开检测从15秒缩短到3秒
- **音频警告**：WiFi连接失败时播放3次800Hz警告蜂鸣音
- **初始化保护**：setup()阶段检测WiFi断开并播放警告

### 💾 内存管理优化
- **DeepSeek API优化**：
  - JSON缓冲区从4KB降至2KB
  - 解析后立即释放responseBody
  - 每个分支调用doc.clear()
- **对话后清理**：
  - 自动释放AI响应和识别结果的String对象
  - 显示对话后内存状态
- **降低阈值**：紧急清理阈值从60KB提升到70KB

### ⏱️ 录音时长调整
- **最大录音时间**：从2秒延长到15秒
- **适用场景**：支持长句子和复杂问题的完整表达
- **超时保护**：自动停止超过15秒的录音

## 技术细节

### 内存使用对比
```
之前配置：
- audioBuffer: 64KB（传统模式）
- JSON缓冲区: 4KB
- 可用内存: ~174KB

当前配置：
- audioBuffer: 4KB（实时模式）
- JSON缓冲区: 2KB
- 可用内存: ~234KB
- 提升: +60KB（34%改善）
```

### 实时识别流程
```
按下按钮 → WebSocket连接（~1s）
         ↓
    LED常亮 + 跳过噪音（<40ms）
         ↓
    开始录音 → I2S读取（2KB/次）
         ↓           ↓
    实时发送    立即释放
         ↓
    松开按钮 → 发送结束帧
         ↓
    获取完整结果 → AI对话 → TTS播放
```

### 音频处理优化
```cpp
// 奇数字节缓冲处理
static uint8_t remainingByte = 0;
static bool hasRemainingByte = false;

// 合并处理
if (hasRemainingByte) {
    merged[0] = remainingByte;
    memcpy(merged + 1, currentData, len);
    send(merged, len + 1);
}

// 保存余数
if (len % 2 != 0) {
    remainingByte = data[len - 1];
    hasRemainingByte = true;
    len = len - 1;
}
```

## 已删除功能

### 移除的代码
- ❌ `useRealtimeASR` 变量和判断
- ❌ `recordBuffer` 和 `recordedSize` 变量
- ❌ `startVoiceRecording()` 函数
- ❌ `stopVoiceRecording()` 函数
- ❌ `processVoiceInput()` 函数
- ❌ `startVoiceConversation()` 函数
- ❌ 串口命令 `record` 和 `stop`
- ❌ 非实时模式的音频缓存逻辑

### 代码体积变化
- Flash大小：973KB → 967KB（减少6KB）
- 函数数量：减少4个
- 代码行数：减少约200行

## 配置变更

### config.h
```cpp
// 之前
#define MAX_RECORD_TIME   2000  // 2秒

// 现在
#define MAX_RECORD_TIME   15000 // 15秒
```

### 音频缓冲区分配
```cpp
// 之前：根据模式动态分配
if (useRealtimeASR) {
    audioBufferSize = 4096;
} else {
    audioBufferSize = 64KB;
}

// 现在：固定4KB
audioBufferSize = 4096;  // 仅实时模式
```

## 性能提升

### 内存效率
- 初始可用内存：238KB
- 对话3次后：>150KB（之前降至49KB）
- 内存泄漏：已修复

### 响应速度
- WiFi断开检测：15s → 3s
- 录音启动延迟：<40ms（之前50ms+）
- WebSocket连接：~1秒

### 音频质量
- TTS噪音：已消除
- 录音完整性：开头数据保护
- 增益控制：1.5倍适度放大

## 兼容性说明

### 硬件要求
- ESP32-S3-N8R2开发板
- INMP441 I2S麦克风
- MAX98357A I2S功放
- GPIO48 LED（无需电阻）
- GPIO1 Boot按钮（录音控制）

### 软件依赖
- PlatformIO
- Arduino框架
- BaiduSpeech库（TTS）
- BaiduRealtimeASR库（实时识别）
- ArduinoJson 6.21.5
- WebSockets 2.7.1

## 已知问题

### 待优化项
- [ ] 长时间运行后内存碎片化
- [ ] WebSocket偶尔断开需要重连
- [ ] 复杂句子识别准确率待提升

### 建议改进
1. 增加内存池管理
2. 实现WebSocket自动重连
3. 优化音频预处理（降噪、增强）

## 升级指南

### 从旧版本升级
1. 备份当前代码
2. 拉取最新代码
3. 更新config.h配置
4. 重新编译上传
5. 测试所有功能

### 配置迁移
```cpp
// 不再需要的配置
// bool useRealtimeASR = true;  // 已删除

// 需要更新的配置
#define MAX_RECORD_TIME   15000  // 2000 → 15000
```

## 测试建议

### 功能测试清单
- [x] 按钮录音（短句）
- [x] 按钮录音（长句，10秒+）
- [x] WiFi断开检测和警告音
- [x] 内存使用监控
- [x] 连续对话（5次+）
- [x] TTS音质测试
- [x] 实时识别准确性

### 性能测试
```bash
# 内存监控
[MEMORY] 对话结束后内存: 180000 bytes (175.8 KB)

# 实时数据传输
[REALTIME-DEBUG] 已发送 50 次音频数据，总计 51200 bytes，内存: 190000 bytes

# 音频电平
[REALTIME-DEBUG] 音频电平 - 平均: 1200, 最大: 8500
```

## 贡献者
- 核心开发：GitHub Copilot
- 测试反馈：用户
- 日期：2025年11月21日

## 相关文档
- [快速开始](./setup/QUICKSTART.md)
- [实时识别指南](./ai/REALTIME_ASR_GUIDE.md)
- [故障排除](./troubleshooting/)
- [项目状态](./PROJECT_STATUS.md)
