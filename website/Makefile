# Minimal Sphinx Makefile (Linux/macOS)
#
# Usage:
#   make html
#   make clean
#

SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = zh/docs
BUILDDIR      = _build

.PHONY: help clean html linkcheck

help:
	@echo "Targets:"
	@echo "  html        Build HTML docs"
	@echo "  clean       Remove build output"
	@echo "  linkcheck   Run Sphinx link checker"

clean:
	rm -rf "$(BUILDDIR)"

html: zh/docs/api/func_api.md
	$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS)

linkcheck:
	$(SPHINXBUILD) -M linkcheck "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS)

zh/docs/api/func_api.md:
	python3 tools/gen_func_api.py --repo-root .. --out zh/docs/api/func_api.md
