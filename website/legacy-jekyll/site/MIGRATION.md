# 网站迁移完成说明

## ✅ 已完成的工作

1. **创建了独立的 `website/` 文件夹**
   - 所有网站相关文件已移动到 `website/` 目录
   - 保持了清晰的目录结构

2. **切换到 Just the Docs 主题**
   - 这是最流行的开源项目文档模板之一
   - 被 GitHub Actions、GitHub CLI 等众多项目使用
   - 提供现代化的界面和优秀的用户体验

3. **重新组织了文档结构**
   ```
   website/
   ├── _config.yml          # Jekyll 配置
   ├── _pages/              # 页面配置
   │   └── navigation.yml  # 导航配置
   ├── docs/                # 文档文件
   │   ├── user-guide.md
   │   ├── developer-guide.md
   │   ├── contributing.md
   │   ├── environment-variables.md
   │   ├── roadmap.md
   │   ├── open-source-checklist.md
   │   └── code-review.md
   ├── index.md             # 首页
   ├── 404.html            # 404 页面
   └── Gemfile             # Ruby 依赖
   ```

4. **更新了 GitHub Actions 工作流**
   - 工作流现在指向 `website/` 目录
   - 路径已更新为 `website/**`

## 🎨 Just the Docs 主题特性

- ✅ **侧边栏导航** - 清晰的文档结构
- ✅ **搜索功能** - 快速查找内容
- ✅ **响应式设计** - 移动端友好
- ✅ **代码高亮** - 语法高亮支持
- ✅ **深色模式** - 可选的深色主题
- ✅ **快速加载** - 优化的性能

## 🚀 本地开发

### 启动服务器

```bash
cd website
bundle install
bundle exec jekyll serve
```

访问: http://localhost:4000/shm-pcc-sdk/

### 构建网站

```bash
cd website
bundle exec jekyll build
```

## 📝 添加新文档

1. 在 `website/docs/` 目录下创建新的 `.md` 文件
2. 添加 front matter：

```markdown
---
layout: default
title: 文档标题
nav_order: 1
parent: 父级分类
description: 文档描述
---

# 文档标题

内容...
```

3. 在 `website/_pages/navigation.yml` 中添加导航链接

## 🔄 从旧结构迁移

旧的 `docs/` 目录中的文件已移动到 `website/` 目录：

- `docs/*.md` → `website/docs/*.md`
- `docs/_config.yml` → `website/_config.yml`
- `docs/Gemfile` → `website/Gemfile`

**注意**: 旧的 `docs/` 目录可以保留用于其他用途，或者删除。

## 🌐 部署

推送到 GitHub 后，GitHub Actions 会自动构建和部署：

1. 提交更改：
   ```bash
   git add website/
   git add .github/workflows/pages.yml
   git commit -m "Migrate website to Just the Docs theme"
   git push origin main
   ```

2. 访问部署的网站：
   https://promisivia.github.io/shm-pcc-sdk/

## 📚 更多信息

- [Just the Docs 文档](https://just-the-docs.github.io/just-the-docs/)
- [Jekyll 文档](https://jekyllrb.com/docs/)

---

**迁移完成时间**: 2024-12-25




