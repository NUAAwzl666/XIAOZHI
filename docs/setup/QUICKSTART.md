# 快速开始指南

## 第一步：配置API密钥

1. 复制配置文件模板：
   ```
   将 include/config.example.h 复制为 include/config.h
   ```

2. 编辑 `include/config.h` 文件，填入您的配置：

### WiFi配置
```cpp
#define WIFI_SSID "您的WiFi名称"
#define WIFI_PASSWORD "您的WiFi密码"
```

### DeepSeek AI配置
1. 访问 https://platform.deepseek.com
2. 注册账号并获取API密钥
3. 填入配置：
```cpp
#define DEEPSEEK_API_KEY "sk-your-api-key-here"
```

### 百度语音服务配置
1. 访问 https://cloud.baidu.com
2. 开通语音技术服务
3. 创建应用获取配置信息：
```cpp
#define BAIDU_APP_ID "your-app-id"
#define BAIDU_API_KEY "your-api-key"
#define BAIDU_SECRET_KEY "your-secret-key"
```

## 第二步：硬件连接

### 麦克风连接 (441模块)
- VCC → 3.3V
- GND → GND
- WS → GPIO45
- SCK → GPIO21
- SD → GPIO47

### 功放连接 (98357模块)
- VCC → 5V
- GND → GND
- LRC → GPIO42
- BCLK → GPIO41
- DIN → GPIO40

### LED连接 (红色二极管)

**如果有220Ω电阻 (推荐)**:
- 长脚(正极) → 220Ω电阻 → GPIO48
- 短脚(负极) → GND

**如果没有电阻 (应急方案)**:
- 长脚(正极) → GPIO48 (直接连接)
- 短脚(负极) → GND

**说明**: 项目已配置软件限流，将LED亮度限制在30%以保护硬件。虽然可以直接连接，但仍建议使用电阻以获得最佳保护效果。

## 第三步：编译上传

1. 确保PlatformIO已安装
2. 在VS Code中打开项目
3. 点击底部状态栏的"上传"按钮，或按 Ctrl+Alt+U

## 第四步：测试和监控

1. 打开串口监视器 (115200波特率)
2. 等待系统启动，观察详细的启动信息
3. 系统会显示硬件信息、WiFi连接状态等
4. 每30秒会有心跳信息显示系统运行状态

### 串口输出说明

系统启动时会显示：
- 硬件信息（芯片型号、闪存大小、CPU频率等）
- WiFi连接过程和结果
- LED测试序列
- 系统就绪提示

运行时会显示：
- 心跳信息（每30秒）
- WiFi状态变化提醒
- 命令执行日志
- 内存使用警告（如果内存不足）

## 常用调试命令

- `status` - 显示详细系统状态（推荐！）
- `wifi` - 显示WiFi连接信息
- `memory` - 显示内存使用情况
- `led` - 运行LED测试序列
- `restart` - 重启系统
- `help` - 显示所有可用命令

### 系统状态监控

系统会自动监控并报告：
- **心跳信息**: 每30秒显示运行时间
- **WiFi状态**: 连接断开时自动提醒
- **内存监控**: 内存不足时发出警告
- **命令计数**: 跟踪已处理的命令数量

说"小智"或"你好"唤醒助手（基础版本暂不支持语音功能）

## 故障排除

1. **编译错误**: 检查是否填写了config.h配置
2. **WiFi连接失败**: 检查SSID和密码
3. **语音服务失败**: 检查API密钥配置
4. **硬件无响应**: 检查引脚连接

---

祝您使用愉快！如有问题请查看README.md或提交Issue。
