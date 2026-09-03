# Build the bilingual Sphinx site.

SPHINXOPTS  ?=
SPHINXBUILD ?= sphinx-build
BUILDDIR    = _build
ZH_SOURCE   = zh/docs
EN_SOURCE   = en/docs
SITE_DIR    = $(BUILDDIR)/html

.PHONY: help clean html zh en linkcheck

help:
	@echo "Targets:"
	@echo "  html        Build the Chinese and English HTML sites"
	@echo "  zh          Build the Chinese site"
	@echo "  en          Build the English site"
	@echo "  clean       Remove build output"
	@echo "  linkcheck   Run Sphinx link checker"

clean:
	rm -rf "$(BUILDDIR)"

html: zh/docs/api/func_api.md
	$(MAKE) zh
	$(MAKE) en
	mkdir -p "$(SITE_DIR)"
	cp landing.html "$(SITE_DIR)/index.html"
	cp 404.html "$(SITE_DIR)/404.html"
	cp favicon.png "$(SITE_DIR)/favicon.png"

zh: zh/docs/api/func_api.md
	$(SPHINXBUILD) -b html "$(ZH_SOURCE)" "$(SITE_DIR)/zh" $(SPHINXOPTS)

en:
	$(SPHINXBUILD) -b html "$(EN_SOURCE)" "$(SITE_DIR)/en" $(SPHINXOPTS)

linkcheck:
	$(SPHINXBUILD) -b linkcheck "$(ZH_SOURCE)" "$(BUILDDIR)/linkcheck-zh" $(SPHINXOPTS)
	$(SPHINXBUILD) -b linkcheck "$(EN_SOURCE)" "$(BUILDDIR)/linkcheck-en" $(SPHINXOPTS)

zh/docs/api/func_api.md:
	python3 tools/gen_func_api.py --repo-root .. --out zh/docs/api/func_api.md
