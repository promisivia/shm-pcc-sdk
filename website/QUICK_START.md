# Documentation Website Quick Start

```bash
cd website
./serve.sh
```

Open `http://localhost:8000/`. The root page links to both language versions:
`/zh/` for Chinese and `/en/` for English.

To create static output without serving it, run `make html`. The output is in
`website/_build/html/`. Set `HOST`, `PORT`, or `VENV` when invoking
`serve.sh` to override its defaults.
