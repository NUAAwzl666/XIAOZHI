# ESP32-S3 智能语音助手"小智"

基于ESP32-S3的实时语音识别AI助手，支持边录边识别、DeepSeek AI对话和百度TTS语音合成。

## ✨ 主要特性

- 🎤 **实时语音识别**：百度实时ASR，边录边识别，低延迟
- 🤖 **AI智能对话**：DeepSeek API，简洁准确的回复
- 🔊 **流式语音合成**：百度TTS流式播放，即合成即播放
- 💾 **低内存占用**：4KB音频缓冲，实时流式处理，可用内存234KB
- 📡 **WiFi智能管理**：3秒断开检测，音频警告提示
- 🎛️ **一键操作**：Boot按钮录音，LED状态指示

## 🔧 硬件清单

| 组件 | 型号 | 用途 |
|------|------|------|
| 开发板 | ESP32-S3-DevKitC-1-N8 | 主控（8MB Flash）|
| 麦克风 | INMP441 | I2S数字麦克风 |
| 功放 | MAX98357A | I2S音频功放 |
| LED | 任意颜色 | 状态指示（GPIO48，无需电阻）|
| 按钮 | Boot按钮 | 录音控制（GPIO1）|

## 📊 系统性能

### 内存使用
```
初始可用：238KB
运行时：  234KB（4KB缓冲区）
对话后：  >150KB（稳定）
提升：    +60KB vs 旧版本（34%改善）
```

### 响应速度
```
WiFi检测：  3秒
录音启动：  <40ms
WebSocket： ~1秒
识别延迟：  实时（<100ms/块）
```

### 音频质量
```
采样率：    16kHz
位深：      16-bit
声道：      单声道（播放时扩展为立体声）
增益：      1.5倍（适度放大，无失真）
```

## 引脚连接

### I2S麦克风 (INMP441)
- WS (LRCLK): GPIO45
- SCK (BCLK): GPIO21
- SD (DIN): GPIO47
- VDD: 3.3V
- GND: GND

### I2S音频功放 (MAX98357A)
- LRC: GPIO42
- BCLK: GPIO41
- DIN: GPIO40
- VIN: 5V
- GND: GND

### LED指示灯
- GPIO48 → LED正极
- GND → LED负极（无需电阻）


## 🚀 快速开始

### 1. 安装PlatformIO
```bash
# VS Code中安装PlatformIO扩展
# 或使用命令行
pip install platformio
```

### 2. 克隆项目
```bash
git clone https://github.com/your-repo/xiaozhi-ai.git
cd xiaozhi-ai
```

### 3. 配置API密钥
```bash
# 复制配置模板
cp include/config.example.h include/config.h

# 编辑config.h，填入你的API密钥：
# - WiFi SSID和密码
# - DeepSeek API密钥
# - 百度AI API密钥和Secret Key
```

### 4. 编译上传
```bash
# 编译
pio run

# 上传
pio run --target upload

# 监控串口
pio device monitor
```

## 💬 使用方法

### 基本操作
1. **开机**：上电后自动连接WiFi
2. **录音**：按住Boot按钮说话（LED常亮）
3. **识别**：松开按钮，系统自动识别并回复
4. **播放**：扬声器播放AI回复（LED闪烁3次表示完成）

### 串口命令
```bash
help         # 显示帮助信息
status       # 系统状态
wifi         # WiFi信息
memory       # 内存信息
restart      # 重启系统
ping         # 测试响应
ttsstream 你好 # 测试TTS播放
```

### LED状态
```
熄灭     - 待机
常亮     - 正在录音
快闪     - 错误/警告
慢闪3次  - 对话完成
```

## 📁 项目结构

```
xiaozhi-ai/
├── src/
│   └── main.cpp              # 主程序
├── include/
│   ├── config.h              # 配置文件（API密钥）
│   └── config.example.h      # 配置模板
├── lib/
│   ├── BaiduSpeech/          # 百度语音库（TTS）
│   ├── BaiduRealtimeASR/     # 百度实时识别库
│   ├── DeepSeekClient/       # DeepSeek AI客户端
│   ├── AudioManager/         # 音频管理
│   └── LEDManager/           # LED控制
├── docs/                     # 文档目录
│   ├── README.md            # 文档索引
│   ├── CHANGELOG_2025-11-21.md  # 最新更新日志
│   ├── setup/               # 安装指南
│   ├── hardware/            # 硬件指南
│   ├── ai/                  # AI功能文档
│   └── troubleshooting/     # 故障排除
├── platformio.ini           # PlatformIO配置
└── README.md               # 本文件
```

## 🔄 最近更新（2025-11-21）

### 重大改进
- ✅ **删除非实时模式**：仅保留实时流式识别
- ✅ **内存优化**：可用内存提升60KB（34%）
- ✅ **TTS噪音修复**：消除音频噪音和失真
- ✅ **WiFi快速检测**：3秒检测+音频警告
- ✅ **录音时长延长**：15秒上限

### 技术亮点
```cpp
// 实时流式处理
audioBufferSize = 4096;  // 仅4KB
边录边发 + 栈上临时缓冲 = 零累积

// 内存及时释放
aiResponse = String();
fullRecognizedText = String();
doc.clear();

// 奇数字节对齐
if (hasRemainingByte) {
    merged[0] = remainingByte;
    memcpy(merged + 1, data, len);
}
```

详见：[更新日志](./docs/CHANGELOG_2025-11-21.md)

## 📖 文档

- [快速开始](./docs/setup/QUICKSTART.md)
- [实时识别指南](./docs/ai/REALTIME_ASR_GUIDE.md)
- [内存优化方案](./docs/ai/MEMORY_OPTIMIZATION.md)
- [硬件连接](./docs/hardware/EXTERNAL_BUTTON_GUIDE.md)
- [故障排除](./docs/troubleshooting/)
- [完整文档索引](./docs/README.md)

## 🐛 常见问题

### WiFi连接失败
```bash
# 检查配置
确认config.h中SSID和密码正确
检查WiFi信号强度

# 播放警告音
系统会播放3次800Hz蜂鸣音
```

### 识别不准确
```bash
# 优化建议
1. 靠近麦克风10-20cm
2. 环境安静，减少噪音
3. 说话清晰，语速适中
4. 录音时长2-5秒最佳
```

### 内存不足
```bash
# 监控内存
输入 memory 查看内存状态

# 低于70KB时自动清理
[MAINTENANCE] 内存不足，执行紧急清理...

# 建议每24小时重启一次
```

### 音频噪音
```bash
# 已修复的问题
✓ TTS奇数字节对齐
✓ 增益过高失真
✓ DMA缓冲区噪音

# 如仍有噪音
1. 检查功放供电（5V稳定）
2. 检查地线连接
3. 降低音量测试
```

## 🤝 贡献

欢迎提交Issue和Pull Request！

### 开发流程
1. Fork项目
2. 创建特性分支
3. 提交代码
4. 推送到分支
5. 创建Pull Request

### 代码规范
- 使用Arduino风格
- 添加注释说明
- 更新相关文档

## 📄 许可证

MIT License

## 🙏 致谢

- [ESP32-Arduino](https://github.com/espressif/arduino-esp32)
- [百度AI开放平台](https://ai.baidu.com/)
- [DeepSeek AI](https://www.deepseek.com/)
- [PlatformIO](https://platformio.org/)

## 📞 联系方式

- Issue: [GitHub Issues](https://github.com/your-repo/xiaozhi-ai/issues)
- 文档: [docs/](./docs/)

---

**小智AI** - 让语音交互更简单 🎙️✨
1. 注册DeepSeek账号：https://platform.deepseek.com
2. 获取API密钥
3. 在`include/config.h`中配置`DEEPSEEK_API_KEY`

### 百度语音服务
1. 注册百度智能云账号：https://cloud.baidu.com
2. 创建语音技术应用
3. 获取APP ID、API Key、Secret Key
4. 在`include/config.h`中配置相关参数

## 开发环境

- **IDE**: VS Code + PlatformIO
- **框架**: Arduino Framework
- **平台**: ESP32 (espressif32)
- **编程语言**: C++

## 安装步骤

1. **克隆项目**
   ```bash
   git clone <项目地址>
   cd XIAOZHI_ai
   ```

2. **配置参数**
   编辑`include/config.h`文件，配置以下参数：
   - WiFi SSID和密码
   - DeepSeek API密钥
   - 百度语音服务配置

3. **安装依赖**
   PlatformIO会自动安装以下库：
   - WiFi
   - HTTPClient
   - ArduinoJson
   - ESP32-audioI2S
   - FastLED
   - WiFiClientSecure

4. **编译上传**
   ```bash
   pio run --target upload
   ```

5. **监控串口**
   ```bash
   pio device monitor
   ```

## 使用方法

### 基本操作

1. **启动设备**: 上电后等待WiFi连接成功
2. **按钮控制**: 
   - 按下GPIO1按钮开始录音(LED常亮)
   - 松开按钮停止录音并开始识别
3. **系统初始化**: 
   - 首次启动或WiFi重连后自动初始化AI服务
   - 初始化完成后播放语音"初始化已完成，现在可以开始了"
4. **语音交互**: 按钮录音 → 语音识别 → AI对话 → 语音播放
5. **串口对话**: 直接在串口输入中文文本与AI对话

### WiFi网络监控

系统具备完善的WiFi监控和提示功能:

**网络检测**:
- 每3秒自动检查WiFi状态
- 快速响应网络断开(最快3秒内)
- 初始化期间实时监控WiFi

**断网提示**:
- 🔊 **声音警告**: 3声蜂鸣提示音(800Hz, 每声600ms)
- 💡 **LED指示**: 快速闪烁表示网络异常
- 📝 **串口提示**: "[WIFI] ⚠ 网络连接已断开，请检查"

**智能处理**:
- 初始化期间检测断网立即停止
- 网络恢复后自动重连
- 自动重新初始化所有服务

### LED状态指示

- **熄灭**: 待机状态
- **常亮**: 正在录音
- **快速闪烁**: WiFi连接中或网络断开
- **5次快闪**: 系统初始化完成

### 音频提示说明

- **3声蜂鸣(哔—哔—哔)**: WiFi网络断开警告
- **语音"初始化已完成，现在可以开始了"**: 所有服务就绪

## 串口调试命令

连接串口(115200波特率)后，可使用以下命令：

- `status` - 显示系统状态
- `restart` - 重启系统
- `test` - 运行系统测试
- `say <文本>` - 测试语音合成
- `listen` - 测试录音功能
- `help` - 显示帮助信息

## 文档与项目结构

### 文档入口
- 查看更详细的使用指南与开发文档，请访问 docs/ 目录：
   - 文档索引: docs/README.md
   - 快速开始: docs/setup/QUICKSTART.md
   - PlatformIO设置: docs/setup/PLATFORMIO_SETUP.md
   - 硬件指南(LED/按钮): docs/hardware/
   - 故障排除: docs/troubleshooting/
   - AI集成说明: docs/ai/

### 项目结构

```
XIAOZHI_ai/
├── .github/
│   └── copilot-instructions.md    # Copilot指令文件
├── include/
│   ├── config.h                   # 配置文件
│   ├── AudioManager.h            # 音频管理器
│   ├── LEDManager.h              # LED管理器
│   ├── DeepSeekClient.h          # DeepSeek客户端
│   ├── BaiduSpeech.h             # 百度语音服务
│   └── VoiceAssistant.h          # 语音助手控制器
├── lib/
│   ├── AudioManager/             # 音频管理器实现
│   ├── LEDManager/               # LED管理器实现
│   ├── DeepSeekClient/           # DeepSeek客户端实现
│   ├── BaiduSpeech/              # 百度语音服务实现
│   └── VoiceAssistant/           # 语音助手控制器实现
├── src/
│   └── main.cpp                  # 主程序
├── platformio.ini                # PlatformIO配置
└── README.md                     # 项目说明（文档入口在 docs/）
```

## 工作流程

1. **初始化阶段**: 启动WiFi、音频模块、LED、AI服务
2. **待机状态**: 持续监听环境声音，检测唤醒词
3. **监听状态**: 检测到唤醒词后开始录音
4. **处理状态**: 语音识别→AI对话→语音合成
5. **播放状态**: 播放AI响应的语音
6. **返回待机**: 完成对话后返回待机状态

## 故障排除

### 常见问题

1. **WiFi连接失败**
   - 检查SSID和密码配置
   - 确认WiFi信号强度
   - 尝试重启设备

2. **语音识别失败**
   - 检查百度语音服务配置
   - 确认网络连接正常
   - 检查麦克风连接

3. **语音合成失败**
   - 检查百度语音服务配置
   - 确认API配额充足
   - 检查扬声器连接

4. **LED不亮**
   - 检查LED引脚连接
   - 确认LED类型配置正确

### 调试技巧

- 使用串口监控查看详细日志
- 使用`status`命令检查系统状态
- 使用`test`命令运行硬件测试
- 检查内存使用情况避免内存不足

## 扩展功能

- [ ] 离线唤醒词检测
- [ ] 多轮对话支持
- [ ] 个性化配置
- [ ] OTA固件更新
- [ ] 本地音频缓存
- [ ] 多语言支持

## 许可证

本项目基于MIT许可证开源。

## 贡献

欢迎提交Issue和Pull Request来改进本项目。

## 联系方式

如有问题，请通过以下方式联系：
- GitHub Issues
- 邮箱: [您的邮箱]

---

**注意**: 使用前请确保已正确配置所有API密钥和网络参数。