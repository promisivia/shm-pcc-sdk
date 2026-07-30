# website 维护规则（Sphinx + RTD）

本目录的文档站点风格对齐 NCCL（Sphinx + Read the Docs theme）。维护时遵循以下约定，避免内容/生成物/样式失控。

## 1) 文档源目录与构建产物

- **文档源目录**：`website/zh/docs/`（MyST Markdown + `conf.py`）
- **构建产物**：`website/_build/html/`（不提交）

## 2) 常用命令

- 本地预览（推荐）：`cd website && ./serve.sh`
- 本地构建：`cd website && make html`
- 清理构建产物：`cd website && make clean`

## 3) 写作与组织规则

- 使用 MyST Markdown（`.md`），用 `toctree` 组织导航（参考 `website/zh/docs/index.md`）。
- 文档按功能分两大块：
  - `website/zh/docs/content/`：SDK 内容总览（目录结构、技术路线、组件纵览）
  - `website/zh/docs/components/`：组件使用（数据结构 / RPC / apps）
  - `website/zh/docs/guides/`：开发者文档，不是用户使用这个SDK的文档，而是开发SDK新功能的文档
  - `website/zh/docs/api/`：按头文件/模块写 “API Reference”
- **优先用 `literalinclude` 引用仓库内真实代码**（例如 `tests/YCSB-C/**`、`ds/BwTree/**`、`shm-lib/include/**`），避免复制粘贴后过时。
- 文案必须是**用户可直接阅读**的说明：避免“写作意图/写作过程/像提示词一样的元描述”（例如“本目录作为主入口，先回答……再引导……”这类表述）；用直接的信息表达替代（例如“这里包含：……；你可以从……开始”）。

## 4) func API 页面（自动生成 + 手写说明）

- 完整 func API 页面：`website/zh/docs/api/func_api.md`
- 生成器脚本：`website/tools/gen_func_api.py`
- 规则：
  - `func_api.md` 顶部“手写说明区”可以人工编辑。
  - `<!-- AUTO-GENERATED BELOW. DO NOT EDIT BY HAND. -->` 之后为自动生成区，**禁止手工修改**。
  - 生成命令：`python3 website/tools/gen_func_api.py --repo-root .. --out website/zh/docs/api/func_api.md`
  - `make html` 会自动刷新该页面。

## 5) 风格与前端

- 主题：`sphinx_rtd_theme`（RTD）
- 样式覆盖：`website/zh/docs/_static/custom.css`
- 目标：维持 “RTD/NCCL 相似阅读体验”（左侧树形导航 + 搜索 + 代码块可复制）。
