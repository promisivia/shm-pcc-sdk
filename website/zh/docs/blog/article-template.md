---
orphan: true
---

<div class="post-kicker">XX / Topic</div>

# 文章标题：用一个设计问题开场

<div class="post-meta"><span>状态：草稿</span><span>核心目录：path/</span><span>预计阅读：15 分钟</span></div>

```{note}
这是新文章的写作模板，不作为正式内容发布。复制后请从首页的 hidden toctree 中添加新页。
```

## 本篇要回答的问题

- 问题一
- 问题二
- 问题三

## 一句话解释

用不需要预备知识的语言，先给出可被后文修正和展开的心智模型。

## 背景与约束

说明模块必须面对的硬件、系统、兼容性和 workload 约束。

## 设计主路径

从一个真实入口出发，沿调用链讲清核心抽象。优先用 `literalinclude` 引用仓库内的真实代码。

## 为什么这样设计

分别写清候选方案、选择依据、得到的收益和付出的代价。不确定的历史动机要标注为待验证，不要从代码反推作者意图。

## 怎样验证

给出最小 demo、正确性测试、性能测试和结果解读方法。

## 继续读代码

- 入口文件：`path/to/entry`
- 核心抽象：`path/to/header`
- 测试：`path/to/test`
- 相关文章：{doc}`architecture-map`
