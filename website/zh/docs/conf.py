# Sphinx configuration for CXL-SDK documentation (NCCL-style)

from __future__ import annotations

import os
from datetime import date

project = "CXL-SDK"
author = "CXL-SDK Team"
copyright = f"{date.today().year}, {author}"

extensions = [
    "myst_parser",
    "sphinx_copybutton",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

language = "zh_CN"

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_css_files = ["custom.css"]

html_theme_options = {
    # RTD defaults already feel close to NCCL; keep minimal overrides
    "collapse_navigation": False,
    "navigation_depth": 4,
    "titles_only": False,
}

# MyST (Markdown) settings
myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "tasklist",
    "linkify",
]
myst_heading_anchors = 4

# Keep links readable, similar to docs.nvidia.com pages
pygments_style = "default"
