# Git 环境变量手动配置指南

## 方法1：使用PowerShell命令

### 步骤1：找到Git安装路径
在文件资源管理器中搜索 `git.exe`，找到它所在的目录。
常见路径：
- C:\Program Files\Git\bin
- C:\Program Files\Git\cmd
- C:\Users\你的用户名\AppData\Local\Programs\Git\bin
- D:\Git\bin

### 步骤2：添加到环境变量（替换下面的路径）
```powershell
# 替换这里的路径为你找到的Git路径
$gitPath = "C:\Program Files\Git\bin"

# 获取当前用户PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")

# 添加Git路径
[Environment]::SetEnvironmentVariable("Path", "$currentPath;$gitPath", "User")

# 刷新当前会话
$env:Path = [Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [Environment]::GetEnvironmentVariable("Path","User")

# 测试
git --version
```

### 步骤3：如果还不行，重启PowerShell

## 方法2：通过系统设置（图形界面）

1. 右键"此电脑" → "属性"
2. 点击"高级系统设置"
3. 点击"环境变量"
4. 在"用户变量"中找到"Path"，双击
5. 点击"新建"
6. 输入Git的bin目录路径（例如：C:\Program Files\Git\bin）
7. 点击"确定"保存
8. 重启PowerShell

## 方法3：使用Git Bash

Git安装后会自带Git Bash，可以直接使用：
- 开始菜单搜索"Git Bash"
- 在Git Bash中所有git命令都可用
- 导航到项目目录：`cd "/d/XIAOZHI ai"`

## 验证Git是否配置成功

```powershell
# 检查Git版本
git --version

# 查看Git配置
git config --list

# 设置用户信息（首次使用需要）
git config --global user.name "你的名字"
git config --global user.email "你的邮箱"
```

## 快速提交代码（配置好后）

```bash
# 初始化仓库（如果是新项目）
git init

# 查看状态
git status

# 添加所有文件
git add .

# 提交
git commit -m "feat: 实时模式优化与内存改进"

# 添加远程仓库（替换为你的仓库地址）
git remote add origin https://github.com/你的用户名/xiaozhi-ai.git

# 推送
git push -u origin main
```

## 如果PowerShell一直不行

**直接使用Git Bash更简单！**

1. 打开"开始菜单"
2. 搜索"Git Bash"
3. 运行Git Bash
4. 输入：`cd "/d/XIAOZHI ai"`
5. 然后就可以使用所有git命令了
