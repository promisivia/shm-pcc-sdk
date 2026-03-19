# SHM-PCC-SDK Documentation Website

本站点已切换为 **Sphinx + Read the Docs theme**（风格对齐 NCCL 文档），主要面向 `shm-lib` 的使用者（YCSB / BwTree 等系统开发者）。

## 快速开始

### 一键启动（推荐）

```bash
cd website
./serve.sh
```

默认监听 `0.0.0.0:8000`。可用环境变量覆盖：

- `PORT=9000 ./serve.sh`
- `HOST=127.0.0.1 ./serve.sh`
- `VENV=.venv-docs ./serve.sh`

### 一键构建

```bash
cd website
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
make html
```

构建产物在 `website/_build/html/`。

## 目录结构（Sphinx）

```
website/
  zh/docs/                # 文档源文件（MyST Markdown + conf.py）
    conf.py
    index.md
    guides/
    api/
    _static/custom.css
  tools/gen_func_api.py   # 从 shm-lib/include 自动生成完整 func API
  requirements.txt
  Makefile
  serve.sh
```

## 写文档的约定

- 尽量使用 `literalinclude` 引用仓库内真实代码片段，避免复制粘贴后过时。
- `zh/docs/api/func_api.md` 的下半部分为自动生成区；请只在顶部“手写说明区”补充解释。
