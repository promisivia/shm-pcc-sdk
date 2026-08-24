# Bilingual Documentation

The Sphinx documentation is maintained as two source trees:

```text
website/zh/docs/  # Chinese source, published at /zh/
website/en/docs/  # English source, published at /en/
```

`make html` builds both trees and creates a small language-selection landing
page at the site root. Each rendered site has a link to the other language's
home page. When adding a new page, add it to the relevant `index.md` toctree
and keep its language-specific navigation usable on its own.
