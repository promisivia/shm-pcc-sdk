# Open-source readiness harness

This harness provides a reproducible release-readiness baseline for CXL-SDK. It
uses Python's standard library and common build tools, so local quick checks do
not require an additional package manager.

## Usage

```bash
# Repository, documentation, artifact, and security hygiene checks
./tools/opensource-harness/run.sh

# Quick checks plus clean core build and Sphinx documentation build
./tools/opensource-harness/run.sh --full

# Optional machine-readable output
./tools/opensource-harness/run.sh --full --report /tmp/cxl-sdk-report.json
```

Full mode builds `shm-lib`, the demos, and the YCSB-C `nocc` variant from clean
temporary directories. The `nocc` build compiles all index adapters involved in
the SPORE supplementary material and the cache-bypassing paths, then runs a
file-backed BwTree smoke workload (1,000 loads and 1,000 mixed operations). To
keep that portable check small, it uses a 16-bit BwTree mapping table; the
paper-scale build default remains 24 bits. Full mode also builds and runs the G2
replicated-pointer stress model. It skips the Sphinx build with a warning when
`sphinx-build` is not installed. Install
[`website/requirements.txt`](../../website/requirements.txt) to make
documentation warnings fatal in CI or release environments.

## What it checks

- required community, security, release, attribution, and submodule metadata;
- repository layout and accidental generated/packaged artifacts;
- large tracked files and Git LFS pointers;
- local Markdown links, stale branding, and unfinished placeholders;
- conservative high-confidence secret patterns;
- syntax of maintained shell entry points;
- Git whitespace errors; and
- in full mode, clean core/demo/YCSB-C `nocc` builds, a local NO_CC/BwTree
  runtime smoke, the G2 stress model, and a strict Sphinx build.

Checks should remain deterministic and safe to run from a developer checkout.
Hardware benchmarks, privileged device setup, and destructive cleanup do not
belong in this harness; CI and release notes should report those separately.
