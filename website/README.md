# SHM-PCC-SDK Documentation Website

The documentation site is built with Sphinx and the Read the Docs theme. It
publishes a Chinese site at `/zh/` and an English site at `/en/`; the root URL
redirects to Chinese.

## Build locally

```bash
cd website
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
make html
```

The generated site is written to `_build/html/`. Use `./serve.sh` to build it
and serve it locally on port 8000 by default.

## Source layout

```text
website/
  zh/docs/  # Chinese Sphinx source
  en/docs/  # English Sphinx source
```

Keep the two language trees independently navigable. The function API page in
`zh/docs/api/func_api.md` has an auto-generated section; refresh it with
`python tools/gen_func_api.py --repo-root .. --out zh/docs/api/func_api.md`.
