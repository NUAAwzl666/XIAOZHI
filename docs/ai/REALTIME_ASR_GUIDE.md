# WebSocket实时语音识别使用指南

## 🎯 功能特性

### 实时语音识别 vs 传统RAW格式

| 特性 | 实时识别（WebSocket） | RAW格式（HTTP POST） |
|------|---------------------|---------------------|
| **协议** | WSS (WebSocket Secure) | HTTPS |
| **传输方式** | 边录边传流式上传 | 录完后一次性上传 |
| **识别延迟** | 实时返回（<1秒） | 录完后识别（2-5秒） |
| **最大时长** | 无限制（可长时间） | 60秒 |
| **内存占用** | 极低（流式传输） | 中等（64KB缓冲） |
| **用户体验** | 边说边显示结果 | 说完等待结果 |
| **适用场景** | 长对话、会议、字幕 | 短命令、快速交互 |
| **实现复杂度** | 较高（WebSocket） | 简单（HTTP） |

## 🔧 配置选项

### 切换识别模式

在`src/main.cpp`中有一个全局变量控制模式：

```cpp
bool useRealtimeASR = true;  // true=实时识别, false=RAW格式
```

**修改方式**：
1. 打开 `src/main.cpp`
2. 找到 `bool useRealtimeASR = true;`
3. 改为 `false` 则使用传统RAW格式
4. 改为 `true` 则使用实时识别

## 📡 WebSocket实时识别工作流程

### 1. 按下按钮
```
用户按下按钮
  ↓
LED灯亮起（表示录音中）
  ↓
建立WebSocket连接到 wss://vop.baidu.com/realtime_asr
  ↓
发送START帧（包含appid, appkey, dev_pid等参数）
  ↓
开始录音并实时发送音频数据
```

### 2. 说话过程
```
I2S麦克风采集音频（32位）
  ↓
转换为16位PCM数据
  ↓
每512样本（~32ms）发送一次
  ↓
服务器返回临时识别结果（MID_TEXT）
  ↓
显示在串口："[实时] 你好"
  ↓
继续说话...
  ↓
服务器返回更新结果："[实时] 你好我是"
```

### 3. 松开按钮
```
发送FINISH帧
  ↓
LED闪烁
  ↓
服务器返回最终识别结果（FIN_TEXT）
  ↓
显示："[最终] 你好我是小智助手 [0-2300 ms]"
  ↓
断开WebSocket连接
  ↓
发送给DeepSeek AI生成回复
  ↓
语音播报回复
  ↓
LED熄灭
```

## 🎬 使用示例

### 实时识别模式测试

```
[用户操作]
1. 按下按钮
2. 说："你好，今天天气怎么样？"
3. 松开按钮

[串口输出]
[REALTIME] 开始实时录音...
[REALTIME-ASR] 正在连接WebSocket服务器...
[REALTIME-ASR] ✓ WebSocket连接成功
[实时识别] WebSocket已连接，可以开始说话

[实时] 你
[实时] 你好
[实时] 你好今天
[实时] 你好今天天气
[实时] 你好今天天气怎么样

[最终] 你好，今天天气怎么样？ [120-3450 ms]
[完整识别] 你好，今天天气怎么样？

[AI] 正在生成回复...
[AI] ✓ 回复: 今天天气晴朗，温度适宜。
[TTS] 播放AI回复...
```

## 📊 内存使用对比

### 实时识别模式
- **WebSocket连接**: ~15KB
- **音频临时缓冲**: ~2KB（512样本）
- **识别结果缓存**: ~1KB
- **总计**: ~18KB ✅ 极低内存

### RAW格式模式
- **音频缓冲**: 64KB（2秒）
- **HTTP连接**: ~20KB
- **总计**: ~84KB ✅ 已优化

### 原JSON+Base64模式（已废弃）
- **音频缓冲**: 160KB（5秒）
- **Base64编码**: 213KB
- **JSON封装**: 2KB
- **HTTP/SSL**: ~30KB
- **总计**: ~245KB ❌ 内存不足

## 🔍 实时识别API详解

### START帧（连接后立即发送）
```json
{
    "type": "START",
    "data": {
        "appid": 105xxx17,
        "appkey": "UA4oPSxxxxkGOuFbb6",
        "dev_pid": 15372,
        "cuid": "esp32_s3_voice_assistant",
        "format": "pcm",
        "sample": 16000
    }
}
```

### 音频数据帧（Binary格式）
- **类型**: Binary (Opcode 0x2)
- **频率**: 每32ms发送一次（512样本）
- **大小**: 1024字节（512样本 × 2字节）
- **格式**: 16位PCM，16kHz，单声道
- **间隔**: 实时发送，无需sleep

### FINISH帧（松开按钮时发送）
```json
{
    "type": "FINISH"
}
```

### 识别结果（Text格式）

**临时结果**:
```json
{
    "err_no": 0,
    "err_msg": "OK",
    "type": "MID_TEXT",
    "result": "你好今天",
    "log_id": 45677785,
    "sn": "session-id_ws_2"
}
```

**最终结果**:
```json
{
    "err_no": 0,
    "err_msg": "OK",
    "type": "FIN_TEXT",
    "result": "你好，今天天气怎么样？",
    "start_time": 120,
    "end_time": 3450,
    "log_id": 45677785,
    "sn": "session-id_ws_2"
}
```

## 🎨 LED指示说明

### 实时识别模式
| LED状态 | 含义 |
|---------|------|
| 熄灭 | 待机 |
| 常亮 | 正在录音 |
| 快速闪烁 | 正在识别（收到临时结果） |
| 3次快闪 | 错误 |
| 熄灭 | 识别完成 |

### RAW格式模式
| LED状态 | 含义 |
|---------|------|
| 熄灭 | 待机 |
| 常亮 | 正在录音 |
| 熄灭 | 处理中 |

## ⚙️ 配置参数

### config.h设置
```cpp
// WiFi配置
#define WIFI_SSID "your_wifi"
#define WIFI_PASSWORD "your_password"

// 百度API
#define BAIDU_APP_ID "105xxx17"
#define BAIDU_API_KEY "UA4oPSxxxxkGOuFbb6"
#define BAIDU_SECRET_KEY "xxxxx"

// 录音参数
#define SAMPLE_RATE 16000       // 固定16kHz
#define MAX_RECORD_TIME 60000   // 实时模式可设置更长（60秒）
```

### 识别模型选择（dev_pid）
| dev_pid | 语言 | 标点 | 说明 |
|---------|------|------|------|
| 1537 | 中文普通话 | 弱标点 | 逗号、句号 |
| **15372** | 中文普通话 | 加强标点 | 逗号、句号、问号、感叹号 ⭐推荐 |
| 15376 | 中文多方言 | 弱标点 | 支持粤语、四川话、东北话 |
| 1737 | 英语 | 无标点 | - |
| 17372 | 英语 | 加强标点 | 逗号、句号、问号 |

## 🐛 调试技巧

### 查看WebSocket连接状态
```cpp
Serial.printf("[DEBUG] WebSocket连接: %s\n", 
              realtimeASR.isConnected() ? "已连接" : "未连接");
```

### 查看识别结果
所有识别结果都会通过回调函数打印到串口：
- `[实时]` - 临时结果
- `[最终]` - 最终结果
- `[完整识别]` - 累积的所有结果

### 错误诊断
```
[错误] 3004: authentication failed
→ API密钥错误，检查config.h

[错误] 3307: speech quality problem  
→ 音频质量差，靠近麦克风

[错误] 3314: no valid data
→ 音频数据无效，检查麦克风连接

WebSocket连接超时
→ 网络问题或服务器繁忙
```

## 🚀 性能优化建议

### 1. 网络优化
- 使用稳定的WiFi连接（信号强度 > -70dBm）
- 避免网络高峰期
- 减少其他设备占用带宽

### 2. 音频质量
- 麦克风距离10-15cm最佳
- 安静环境（背景噪音<50dB）
- 正常说话音量，不要太大声或太小声

### 3. 内存管理
- 实时模式内存占用极低，可以长时间运行
- 定期检查 `ESP.getFreeHeap()`
- 避免频繁创建/销毁大对象

## 📝 常见问题

**Q: 实时识别和RAW格式哪个更好？**  
A: 取决于使用场景：
- 短命令（如"打开灯"）→ RAW格式（更快）
- 长对话（如会议记录）→ 实时识别（体验更好）
- 内存有限 → 实时识别（占用更少）

**Q: 可以同时支持两种模式吗？**  
A: 可以！通过修改 `useRealtimeASR` 变量即可切换，无需重新编译。

**Q: 实时识别会增加流量吗？**  
A: 略有增加，但不显著。实时模式会发送一些控制帧（START, FINISH），但音频数据量相同。

**Q: 如何提高识别率？**  
A: 
1. 使用dev_pid=15372（加强标点）
2. 确保麦克风连接正确（WS=45, SCK=21, SD=47）
3. 安静环境，清晰发音
4. 保持合适距离（10-15cm）

## 📖 参考文档

- [百度实时语音识别API](https://cloud.baidu.com/doc/SPEECH/s/jlbxejt2i)
- [WebSocket RFC6455](https://tools.ietf.org/html/rfc6455)
- [INMP441麦克风数据手册](https://invensense.tdk.com/products/digital/inmp441/)

## 🎉 快速开始

1. 上传固件
   ```bash
   pio run -t upload
   ```

2. 打开串口监视器
   ```bash
   pio device monitor
   ```

3. 等待初始化
   ```
   [REALTIME-ASR] ✓ 初始化成功
   ```

4. 测试对话
   - 按下按钮
   - 说话
   - 松开按钮
   - 查看实时识别结果

5. 观察效果
   - LED常亮 = 正在录音
   - 串口显示实时结果
   - AI生成回复
   - 语音播报

🎊 现在你已经拥有一个支持实时语音识别的智能助手了！
