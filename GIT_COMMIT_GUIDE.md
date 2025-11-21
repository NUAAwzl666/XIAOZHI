# Git提交指南

## 已创建的文档

本次更新创建了以下文档：

1. **docs/CHANGELOG_2025-11-21.md** - 2025年11月21日更新日志
   - 实时语音识别优化
   - 音频系统改进
   - WiFi管理优化
   - 内存管理优化
   - 录音时长调整

2. **docs/ai/MEMORY_OPTIMIZATION.md** - 内存优化方案
   - 优化前后对比
   - 关键优化措施
   - 内存监控方案
   - 最佳实践

3. **docs/README.md** - 更新文档索引
   - 添加内存优化方案链接
   - 添加更新日志链接

4. **README.md** - 更新主README
   - 重新组织结构
   - 添加系统性能指标
   - 添加快速开始指南
   - 添加常见问题解答

## Git提交步骤

### 如果你的系统已安装Git

```bash
# 1. 查看文件状态
git status

# 2. 添加所有更改的文件
git add .

# 3. 提交更改
git commit -m "feat: 实时模式优化与内存改进

主要更新：
- 删除非实时录音模式，仅保留实时流式识别
- 内存优化：可用内存从174KB提升到234KB（+60KB）
- TTS噪音修复：奇数字节对齐处理
- WiFi快速检测：3秒断开检测+音频警告
- 录音时长：从2秒延长到15秒

技术细节：
- audioBuffer从64KB降至4KB
- DeepSeek API JSON缓冲区从4KB降至2KB
- 实时流式发送，无内存累积
- 及时释放String对象和JSON文档

新增文档：
- docs/CHANGELOG_2025-11-21.md
- docs/ai/MEMORY_OPTIMIZATION.md
- 更新README.md和docs/README.md

代码变更：
- 删除约200行非实时模式代码
- Flash大小：973KB → 967KB（-6KB）
- RAM使用：稳定在234KB可用内存"

# 4. 推送到远程仓库
git push origin main
# 或如果你的主分支叫master
git push origin master
```

### 如果你需要先初始化Git仓库

```bash
# 1. 初始化仓库
git init

# 2. 添加远程仓库（替换为你的仓库地址）
git remote add origin https://github.com/your-username/xiaozhi-ai.git

# 3. 添加所有文件
git add .

# 4. 首次提交
git commit -m "feat: ESP32-S3智能语音助手完整版本

完整功能：
- 实时语音识别（百度ASR WebSocket流式）
- AI对话（DeepSeek API）
- 语音合成（百度TTS流式播放）
- WiFi智能管理
- 低内存占用（4KB缓冲区）
- LED状态指示
- Boot按钮控制"

# 5. 推送到远程
git push -u origin main
```

## 文件清单

### 新增文件
```
docs/CHANGELOG_2025-11-21.md          (更新日志)
docs/ai/MEMORY_OPTIMIZATION.md        (内存优化文档)
GIT_COMMIT_GUIDE.md                   (本文件)
```

### 修改文件
```
docs/README.md                        (文档索引)
README.md                            (主README)
```

### 核心代码（应该已在Git中）
```
src/main.cpp                          (主程序)
include/config.h                      (配置文件，注意.gitignore)
include/config.example.h              (配置模板)
platformio.ini                        (PlatformIO配置)
lib/BaiduSpeech/                     (百度语音库)
lib/BaiduRealtimeASR/                (实时识别库)
lib/DeepSeekClient/                  (DeepSeek客户端)
lib/AudioManager/                    (音频管理)
lib/LEDManager/                      (LED控制)
```

## 重要提示

### .gitignore配置
确保你的`.gitignore`包含：
```gitignore
# 敏感配置文件
include/config.h

# PlatformIO
.pio/
.vscode/
*.pyc

# 编译输出
*.bin
*.elf
*.map

# 临时文件
*.tmp
*.log
```

### 保护敏感信息
- `config.h` 包含API密钥，不应提交到Git
- 仅提交 `config.example.h` 作为配置模板
- 用户需要自己复制并填写API密钥

## 建议的Git工作流

### 开发分支策略
```bash
main/master    - 稳定版本
dev           - 开发分支
feature/*     - 功能分支
hotfix/*      - 紧急修复
```

### 提交规范
```
feat: 新功能
fix: 修复bug
docs: 文档更新
style: 代码格式
refactor: 重构
perf: 性能优化
test: 测试相关
chore: 构建/工具配置
```

## 如果系统未安装Git

### Windows安装Git
1. 下载：https://git-scm.com/download/win
2. 运行安装程序
3. 重启PowerShell
4. 验证：`git --version`

### 或使用GitHub Desktop
1. 下载：https://desktop.github.com/
2. 图形化界面，无需命令行
3. 登录GitHub账号
4. Clone/Create Repository
5. 提交更改并推送

## 备用方案：手动上传到GitHub

如果暂时无法使用Git命令：

1. 在GitHub创建新仓库
2. 点击"Upload files"
3. 拖拽文件上传
4. 添加提交信息
5. 点击"Commit changes"

注意：手动上传时请忽略：
- `.pio/` 目录
- `include/config.h` 文件
- `.vscode/` 目录
