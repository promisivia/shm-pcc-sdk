#!/bin/bash

# 批量更新文档添加语言标记和语言切换器

cd "$(dirname "$0")"

echo "更新中文文档..."
find zh/docs -name "*.md" -type f | while read file; do
    # 检查是否已有 language-switcher
    if ! grep -q "language-switcher" "$file"; then
        # 在第一个 # 标题后添加语言切换器
        sed -i '/^# /a\
\
{% include language-switcher.html %}\
' "$file"
    fi
    
    # 更新 front matter 添加 lang 和 permalink
    if ! grep -q "lang: zh" "$file"; then
        sed -i '/^---$/,/^---$/ {
            /^---$/ i\
lang: zh
        }' "$file"
    fi
done

echo "创建英文文档结构..."
# 复制所有中文文档到英文目录（作为模板）
find zh/docs -name "*.md" -type f | while read file; do
    en_file=$(echo "$file" | sed 's|zh/docs|en/docs|')
    mkdir -p "$(dirname "$en_file")"
    if [ ! -f "$en_file" ]; then
        cp "$file" "$en_file"
        # 更新英文版本的 front matter
        sed -i 's/lang: zh/lang: en/' "$en_file"
        sed -i 's|/zh/docs/|/en/docs/|g' "$en_file"
        sed -i 's|parent: 用户文档|parent: User Documentation|' "$en_file"
        sed -i 's|parent: 开发者文档|parent: Developer Documentation|' "$en_file"
        sed -i 's|parent: 项目信息|parent: Project Information|' "$en_file"
    fi
done

echo "完成！"



