# SHM-PCC-SDK Documentation Website

这是 SHM-PCC-SDK 的文档网站，使用 [Just the Docs](https://just-the-docs.github.io/just-the-docs/) 主题构建，支持**中文和英文**双语切换。

## 🌐 多语言支持

网站支持中文和英文两种语言：
- **中文（默认）**: `/shm-pcc-sdk/` 或 `/shm-pcc-sdk/zh/`
- **英文**: `/shm-pcc-sdk/en/`

用户可以在页面右上角使用语言切换器切换语言。

## 🚀 快速开始

### 一键启动（推荐）

```bash
cd website
./serve.sh
```

脚本会自动：
- ✅ 检查 Ruby 和 Bundler 是否安装
- ✅ 安装所有必需的依赖
- ✅ 启动本地服务器
- ✅ 显示访问地址

启动后访问: 
- 中文: http://localhost:4000/shm-pcc-sdk/
- 英文: http://localhost:4000/shm-pcc-sdk/en/

### 一键构建

```bash
cd website
bundle exec jekyll build
```

这会构建静态网站到 `_site/` 目录。

## 📋 手动安装

### 安装依赖

```bash
cd website
bundle config set --local path 'vendor/bundle'
bundle install
```

### 运行本地服务器

```bash
bundle exec jekyll serve
```

或者绑定到所有网络接口：

```bash
bundle exec jekyll serve --host 0.0.0.0
```

## 📁 目录结构

```
website/
├── _config.yml          # Jekyll 配置
├── _includes/           # 包含文件
│   └── language-switcher.html  # 语言切换器
├── _pages/              # 页面配置
│   └── navigation.yml   # 导航配置
├── zh/                  # 中文内容
│   ├── index.md         # 中文首页
│   └── docs/            # 中文文档
├── en/                  # 英文内容
│   ├── index.md         # 英文首页
│   └── docs/            # 英文文档
├── index.md             # 默认首页（中文）
├── serve.sh             # 一键启动脚本
└── Gemfile              # Ruby 依赖
```

## 📝 添加新文档

### 中文文档

1. 在 `zh/docs/` 目录下创建新的 `.md` 文件
2. 添加 front matter：

```markdown
---
layout: default
title: 文档标题
nav_order: 1
parent: 父级分类
description: 文档描述
lang: zh
permalink: /zh/docs/filename.html
---

{% include language-switcher.html %}

# 文档标题

内容...
```

### 英文文档

1. 在 `en/docs/` 目录下创建对应的 `.md` 文件
2. 添加相同的 front matter，但设置 `lang: en` 和英文 permalink

### 批量更新

使用脚本批量更新文档：

```bash
./update_lang.sh
```

## 🎨 主题特性

- ✅ 响应式设计
- ✅ 搜索功能
- ✅ 侧边栏导航
- ✅ 代码高亮
- ✅ 深色模式支持
- ✅ 移动端友好
- ✅ **多语言切换**

## 🔧 故障排除

### 端口被占用

如果默认端口 4000 被占用，脚本会自动尝试使用 4001。

手动指定端口：

```bash
bundle exec jekyll serve --port 4001
```

### 权限问题

如果遇到权限问题，确保使用本地安装的 gems：

```bash
bundle config set --local path 'vendor/bundle'
bundle install
```

### 清除缓存

如果遇到奇怪的问题，清除 Jekyll 缓存：

```bash
rm -rf .jekyll-cache .sass-cache _site assets/css
```

## 📚 更多信息

- [多语言支持说明](MULTILINGUAL.md) - 详细的多语言配置指南
- [Just the Docs 文档](https://just-the-docs.github.io/just-the-docs/)
- [Jekyll 文档](https://jekyllrb.com/docs/)

## 🌐 部署

推送到 GitHub 后，GitHub Actions 会自动构建和部署到 GitHub Pages。

访问地址: 
- 中文: https://promisivia.github.io/shm-pcc-sdk/
- 英文: https://promisivia.github.io/shm-pcc-sdk/en/
