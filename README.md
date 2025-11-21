 # ESP32-S3智能语音助手"小智"

## 项目概述

这是一个基于ESP32-S3-N8R2开发板的智能语音助手项目，集成了多种硬件模块和AI服务，能够实现语音唤醒、语音识别、AI对话和语音合成的完整流程。

## 硬件配置

- **开发板**: ESP32-S3-N8R2 (8MB Flash, 2MB PSRAM)
- **麦克风**: 441麦克风模块 (I2S接口)
- **音频功放**: 98357音频功放模块 (I2S接口)
- **指示灯**: LED灯 (WS2812B)

## 引脚连接

### I2S麦克风 (441模块)
- WS (LRCLK): GPIO45
- SCK (BCLK): GPIO21
- SD (DIN): GPIO47

### I2S音频功放 (98357模块)
- WS (LRCLK): GPIO42
- SCK (BCLK): GPIO41
- SD (DIN): GPIO40

### LED灯 (红色二极管)

**方案一 (推荐)**: 使用限流电阻
- 长脚(正极): GPIO48 → 220Ω电阻 → LED长脚
- 短脚(负极): GND

### 外部按钮 (语音对话触发)
- 按钮一端: GPIO1
- 按钮另一端: GND
- 说明: 使用内部上拉电阻，无需外部电阻

**方案二 (无电阻)**: 软件限流
- 长脚(正极): GPIO48 (直接连接，软件限制亮度到30%)
- 短脚(负极): GND

## 功能特性

## 功能特性

### ✅ 已完成功能
- **硬件驱动**
  - ESP32-S3-N8R2开发板支持
  - 441麦克风模块集成（I2S接口）
  - 98357音频功放模块集成（I2S接口）
  - LED状态指示灯（支持软件限流保护）

- **交互控制**
  - 外部按钮控制语音对话
  - 一键触发语音识别
  - LED状态指示反馈
  - 防抖处理确保稳定性

- **网络连接**
  - WiFi自动连接和重连
  - 网络状态监控
  - 连接质量评估

- **系统监控**
  - 实时内存使用监控
  - 硬件信息显示
  - WiFi连接状态追踪
  - 心跳监控（30秒间隔）
  - 系统状态报告

- **调试功能**
  - 详细的串口输出
  - 交互式命令界面
  - 系统诊断工具
  - 错误日志记录

- **🤖 AI智能对话**
  - DeepSeek AI对话服务 ✅ 已集成
  - 智能中文对话
  - 上下文理解
  - 友好的助手角色
  - 实时对话计数

### 🔄 开发中功能
- **语音识别与合成**
  - 百度语音识别API集成
  - 百度语音合成API集成
  - 语音唤醒检测（"小智"、"你好"等）
  - 完整语音交互流程

### 📋 可用命令
通过串口监视器可以使用以下命令：
- `status` - 显示系统详细状态（包含AI状态）
- `wifi` - 显示WiFi连接信息
- `memory` - 显示内存使用情况
- `led` - 测试LED灯功能
- `ai` - 显示AI服务状态
- `test` - 测试AI服务连接
- `chat [消息]` - 与AI对话（例如：chat 你好）
- `help` - 显示所有可用命令
- `restart` - 重启系统

### 💬 AI对话功能
- **直接对话**: 在串口中直接输入中文即可与小智对话
- **智能回复**: AI会给出简洁友好的中文回复
- **状态监控**: 实时显示对话次数和AI服务状态

## AI服务配置

### DeepSeek AI
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

1. **启动设备**: 上电后等待WiFi连接成功
2. **语音唤醒**: 说"小智"或"你好"唤醒助手
3. **语音对话**: 唤醒后直接说话，助手会回应
4. **状态指示**: 
   - 关闭: 待机状态
   - 慢呼吸: 监听中
   - 快闪烁: 处理中
   - 脉冲: 播放响应
   - 中速闪烁: 错误状态
   - 慢闪烁: 连接中

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