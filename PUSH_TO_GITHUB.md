# 推送到GitHub步骤

## ✅ 本地提交已完成

你的文档更新已成功提交到本地Git仓库：
- 提交ID: ff96a66
- 新增文件: 4个
- 修改文件: 2个
- 总计: 1183行新增, 99行删除

## 📤 推送到GitHub

### 步骤1：在GitHub创建仓库（如果还没有）

1. 访问 https://github.com/new
2. 仓库名称：`xiaozhi-ai` 或其他名称
3. 选择 Public 或 Private
4. **不要**勾选"Initialize with README"（因为本地已有）
5. 点击"Create repository"

### 步骤2：添加远程仓库

复制GitHub给你的仓库地址（例如：`https://github.com/你的用户名/xiaozhi-ai.git`）

然后运行：

```bash
# 添加远程仓库（替换为你的实际地址）
git remote add origin https://github.com/你的用户名/xiaozhi-ai.git

# 验证配置
git remote -v
```

### 步骤3：推送代码

```bash
# 首次推送（设置upstream）
git push -u origin master

# 后续推送只需要
git push
```

### 如果遇到认证问题

**方法1：使用Personal Access Token（推荐）**

1. GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Generate new token
3. 勾选 `repo` 权限
4. 复制token
5. 推送时用token作为密码：
   ```bash
   用户名：你的GitHub用户名
   密码：粘贴你的token
   ```

**方法2：使用SSH**

```bash
# 生成SSH密钥
ssh-keygen -t rsa -b 4096 -C "your_email@example.com"

# 复制公钥
cat ~/.ssh/id_rsa.pub

# 在GitHub添加SSH密钥（Settings → SSH keys）
# 然后使用SSH地址
git remote set-url origin git@github.com:你的用户名/xiaozhi-ai.git
git push -u origin master
```

## 📋 当前状态

```
本地分支: master
本地提交: ff96a66 (最新)
远程仓库: 未配置

待推送文件：
✓ GIT_COMMIT_GUIDE.md
✓ GIT_PATH_SETUP.md  
✓ README.md (更新)
✓ docs/CHANGELOG_2025-11-21.md
✓ docs/README.md (更新)
✓ docs/ai/MEMORY_OPTIMIZATION.md

未提交文件：
- include/config.h (包含密钥，不应提交)
- src/main.cpp (代码已提交过)
```

## 🔒 重要提示

确保你的 `.gitignore` 包含：
```gitignore
include/config.h
.pio/
.vscode/
```

这样可以防止提交敏感信息（API密钥）。

## ✨ 完成后

推送成功后，你可以在GitHub仓库看到：
- 完整的项目文档
- 详细的更新日志
- 内存优化技术文档
- 清晰的项目结构

---

**准备好推送了吗？** 

创建好GitHub仓库后，告诉我仓库地址，我帮你配置并推送！
