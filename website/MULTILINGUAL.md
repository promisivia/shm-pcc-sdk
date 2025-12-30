# 多语言支持说明

## 🌐 语言支持

网站现在支持中文和英文两种语言，用户可以在页面右上角切换语言。

## 📁 目录结构

```
website/
├── index.md              # 中文首页（默认）
├── en/
│   └── index.md         # 英文首页
├── zh/
│   └── docs/            # 中文文档
│       ├── user-guide.md
│       ├── developer-guide.md
│       └── ...
└── en/
    └── docs/            # 英文文档
        ├── user-guide.md
        ├── developer-guide.md
        └── ...
```

## 🔄 URL 结构

- **中文（默认）**: `/shm-pcc-sdk/` 或 `/shm-pcc-sdk/zh/`
- **英文**: `/shm-pcc-sdk/en/`

## 📝 添加新文档

### 中文文档

1. 在 `zh/docs/` 目录下创建 `.md` 文件
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
2. 添加 front matter：

```markdown
---
layout: default
title: Document Title
nav_order: 1
parent: Parent Category
description: Document description
lang: en
permalink: /en/docs/filename.html
---

{% include language-switcher.html %}

# Document Title

Content...
```

## 🛠️ 批量更新脚本

使用 `update_lang.sh` 脚本可以批量更新文档：

```bash
./update_lang.sh
```

这个脚本会：
- 为所有中文文档添加语言切换器和 lang 标记
- 创建英文文档模板（如果不存在）

## 🎨 语言切换器

语言切换器会自动出现在每个页面的右上角。它使用 JavaScript 检测当前语言并切换 URL。

### 自定义位置

编辑 `_includes/language-switcher.html` 可以自定义语言切换器的样式和位置。

## 📚 导航配置

- `_pages/navigation.yml` - 中文导航配置
- `_pages/navigation_en.yml` - 英文导航配置（可选，如果使用动态导航）

## ✅ 检查清单

添加新文档时，确保：

- [ ] 中文版本在 `zh/docs/` 目录
- [ ] 英文版本在 `en/docs/` 目录
- [ ] 两个版本都有正确的 front matter
- [ ] 两个版本都包含 `{% include language-switcher.html %}`
- [ ] permalink 正确设置（/zh/ 或 /en/ 前缀）
- [ ] lang 字段正确设置（zh 或 en）

## 🔍 测试

构建网站后，检查：

1. 中文页面可以正常访问
2. 英文页面可以正常访问
3. 语言切换器可以正常工作
4. URL 切换正确

```bash
# 构建并测试
bundle exec jekyll build
bundle exec jekyll serve
```

访问：
- http://localhost:4000/shm-pcc-sdk/ （中文）
- http://localhost:4000/shm-pcc-sdk/en/ （英文）



