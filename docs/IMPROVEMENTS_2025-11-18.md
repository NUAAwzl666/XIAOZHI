# 改进功能总结（2025-11-18）

本文档汇总本次迭代在音频实时识别与LED指示方面的改进，覆盖实现背景、变更细节、行为规范、测试方法与注意事项，便于后续维护与回归验证。

## 概览
- 设备：ESP32-S3-N8R2（Arduino 框架）
- 音频：INMP441 I2S 麦克风 → 16kHz/32bit 输入，右移14位转16bit
- 实时ASR：百度 WebSocket 实时识别（wss://vop.baidu.com/realtime_asr）
- 主要改进目标：
  1) 修复“按键开始后首秒音频未上传”的问题
  2) 明确 LED 行为规范（连接中闪烁、按下常亮、松开熄灭）
  3) 提升可靠性与可观测性（缓冲、统计、无阻塞刷新）

---

## 变更清单（高层）
- 实时识别/音频链路
  - 新增“预缓冲”机制：WebSocket 连接建立前的音频先写入内存缓冲，连接就绪后立即补发，避免首秒音频丢失。
  - 循环中根据连接状态动态切换“缓存/直发”。
  - 修正 I2S 声道为 ONLY_RIGHT（适配 INMP441 L/R 接 VDD 场景）。
- LED 行为与状态机
  - 新增轻量 LED 状态机（LED_OFF/LED_ON/LED_BLINK_FAST/LED_BLINK_SLOW）。
  - WiFi 连接/重连时：快闪；连接成功：熄灭（若未录音）。
  - 按住按钮讲话：常亮；松开按钮：熄灭。
  - 移除 WebSocket 连接回调内“强制亮灯”与“部分结果闪烁”的副作用。
  - 在 loop() 顶部调用 updateLED()，实现无阻塞刷新。
- 稳定性与调试
  - 保留音频电平统计、I2S 原始样本打印（可按需启用）。
  - 统一发送统计与错误提示，便于现场诊断。

---

## 代码触及点（参考）
- `src/main.cpp`
  - 录音启动：`startRealtimeRecording()`（先录音缓冲，后连WS并补发）
  - 音频循环：`loop()` → I2S读取→ 32→16bit → 根据 `realtimeASR.isConnected()` 决定缓存或发送
  - LED 状态机：`setLEDMode()` / `updateLED()`；在 `loop()` 顶部每次调用 `updateLED()`
  - 按钮事件：`handleButtonPress()`（通过校验后 LED_ON）、`handleButtonRelease()`（LED_OFF）
  - WiFi：初次连接与 `checkWiFiStatus()` 重连过程中快闪、成功后熄灭（若未录音）
  - WebSocket 回调：`onRealtimeConnected/onRealtimeDisconnected/onRealtimePartialResult` 移除对 LED 的强制控制

---

## LED 行为规范（新）
- 连接过程中
  - WiFi 首次连接/重连：LED_BLINK_FAST（100ms 周期）
- 连接成功
  - 若未在录音：LED_OFF（熄灭）
- 录音过程中
  - 按住按钮：LED_ON（常亮）
  - 松开按钮：LED_OFF（熄灭）
- 错误提示
  - 临时提示使用 LED_BLINK_SLOW（500ms 周期，短时显示）

优先级说明：录音优先级最高。即使系统正在 WiFi 重连，用户按下按钮时 LED 也保持常亮，不被重连快闪覆盖。

---

## 实时ASR与首秒音频修复
- 背景：WebSocket 建连与发送 START 帧需要时间；此前在连接未就绪时采集的音频被丢弃，导致首秒音频缺失。
- 方案：
  1) 按钮按下即开始采集：将 PCM 写入 `audioBuffer` 预缓冲；
  2) 同步发起 WebSocket 连接；
  3) 连接成功后立即把 `recordedSize` 中的预缓冲数据补发；
  4) 连接就绪后音频改为“实时直发”。
- 效果：首秒关键语音可稳定上传，识别准确率提升。

---

## I2S 声道与幅度问题
- 现网硬件接法：INMP441 的 L/R 接 VDD → 右声道有效。
- 配置调整：`I2S_CHANNEL_FMT_ONLY_RIGHT`；32→16bit 右移 14 位（高 18 位有效）。
- 诊断方法：
  - 打开 I2S 原始样本打印与电平统计（串口日志），
  - 说话时最大值应显著高于环境底噪（> 1000）。

---

## 测试与验收建议
- 基线
  1) 上电后 WiFi 连接：LED 快闪 → 连接成功后熄灭；
  2) 按住按钮说话：LED 常亮，串口显示“实时发送”和“音频电平”；
  3) 松开按钮：LED 熄灭；ASR 返回部分与最终结果；
  4) 断开 WiFi 再恢复：断开期间快闪，恢复后熄灭；录音期间不被覆盖。
- 回归用例
  - 首秒语音是否被识别（检验预缓冲补发成功）。
  - WebSocket 连接/断开不再触发 LED 常亮/闪烁副作用。
  - I2S 电平在语音说话时显著提升。

---

## 运维与开关
- 如需暂时关闭实时ASR并使用传统 RAW 方式，可将 `useRealtimeASR` 设为 `false`（`src/main.cpp`）。
- LED 亮度或策略可进一步上移至 `LEDManager`（本项目也包含更完整的 LED 模块，可按需迁移复用）。

---

## 已知限制与后续项
- 预缓冲使用内存缓冲区，请留意 `audioBufferSize` 上限与可用堆内存；
- 若出现环境底噪较高，可考虑简单 VAD 门限或平均值自适应；
- 后续可将 LED 状态机下沉到 `lib/LEDManager`，统一管理全局状态优先级。

---

## 版本信息
- 日期：2025-11-18
- 主要文件：`src/main.cpp`
- 关键关键词：WebSocket 实时识别、预缓冲补发、I2S ONLY_RIGHT、LED 状态机、WiFi 快闪、按钮常亮
