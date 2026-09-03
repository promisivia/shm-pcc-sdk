"""Sphinx configuration for the English CXL-SDK documentation."""

from datetime import date

project = "CXL-SDK"
author = "CXL-SDK Team"
copyright = f"{date.today().year}, {author}"

extensions = ["myst_parser", "sphinx_copybutton"]
templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]
language = "en"

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
html_css_files = ["custom.css"]
html_favicon = "../../favicon.png"
html_theme_options = {
    "collapse_navigation": False,
    "navigation_depth": 4,
    "titles_only": True,
}

myst_enable_extensions = ["colon_fence", "deflist", "tasklist", "linkify"]
# The imported Markdown uses ordinary intra-page links.  MyST renders those
# links correctly, but they are not Sphinx document references to validate.
suppress_warnings = ["myst.xref_missing"]
pygments_style = "default"
