# CXL-SDK open-source readiness report

This document records the repository changes and validation performed for the
2026-07-31 open-source baseline. It is intended to make the release auditable:
every passing claim below is tied to a repeatable command, and checks that need
special hardware or unavailable source material remain explicitly open.

## What changed

### Repository and release hygiene

- Renamed the public-facing project to **CXL-SDK** and rewrote the root README
  around the actual tracked components, configuration, examples, and build
  paths.
- Merged the former `opensource` line into the GitHub `master` line while
  preserving both histories.
- Added contribution, conduct, security, support, changelog, editor, and
  third-party notice files.
- Added issue and pull-request templates plus an open-source readiness GitHub
  Actions workflow; GitHub Pages now follows `master`.
- Removed stale submodule metadata and generated binaries, object archives,
  Python bytecode, and unavailable Git LFS package pointers from the tracked
  tree. These files remain recoverable from Git history.

### Build and code fixes

- Made libssh discovery produce a stable `Libssh::Libssh` CMake target.
- Restored the x86 non-temporal read/write helpers used by the runtime.
- Added missing shared-memory allocator declarations to BTreeOLC, Masstree,
  CLHT support code, and related benchmark build paths.
- Added a tracked demo CMake project so the documented clean demo build works.
- Removed unconditional debug output from the production G2 helper.
- Made the G2 stress model bounded and repeatable, avoided the zero-value test
  collision, added a quiescent replica-convergence check, and made its TLA+
  Makefile accept a portable `TLA2TOOLS_JAR` setting.

### Documentation

- Replaced stale branding, placeholder repository URLs, unfinished template
  text, invalid documentation directives, and broken local links.
- Repaired the Sphinx document tree so `sphinx-build -n -W` is warning-free.
- Documented the supported public allocator path and clearly marked external
  allocator integrations as optional rather than bundled.
- Added this report and a repository-owned readiness harness.

## Repeatable validation

Run the fast checks during development:

```bash
./tools/opensource-harness/run.sh
```

Run the release baseline from clean temporary build directories:

```bash
./tools/opensource-harness/run.sh --full \
  --report /tmp/cxl-sdk-readiness.json
```

Full mode checks repository metadata, generated artifacts, large files, local
links, branding placeholders, common secret formats, shell syntax, and Git
whitespace. It then performs clean builds of the runtime, demos, and the
YCSB-C `nocc` variant, runs the G2 replicated-pointer stress model, and builds
the Chinese Sphinx site with warnings treated as errors.

## SPORE supplementary-material traceability

The locally available `supplementary-material.pdf` has SHA-256
`bad4f149af2531b2c20136be8d78f2aab77879348ed156e8555ee023d14ae9b9` and is
titled *Indexing Made Consistent and Efficient on Partially Cache Coherent
Shared Memory with SPORE Guidelines*. The table records what can be established
in a portable CI host.

| Supplement section | Repository evidence | Current validation |
| --- | --- | --- |
| §A.1 optimistic locks: BTreeOLC, ART OLC, Masstree | `ds/BTreeOLC/`, `ds/RadixART/OptimisticLockCoupling/`, `ds/Masstree/`; non-temporal version fields and node flush/invalidation paths | All adapters compile in the clean YCSB-C `nocc` build |
| §A.2 lock-free indexes: BwTree and ClevelHash | `ds/BwTree/`, `ds/ClevelHash/`; cache-bypassing atomics and replicated metadata integration | Both adapters compile in the clean `nocc` build |
| §A.3 ROWEX indexes: CLHT and HOT | `ds/CLHT/`, `ds/HOT/`; cache-bypassing key/value/pointer and writer synchronization paths | Both adapters compile in the clean `nocc` build |
| §B G2 replicated pointers | `shm-lib/include/replica_help_update/help_update.h`, `ds/ClevelHash/clevel_hash.hpp`, and `tests/correctness/help_update_verify/` | Production integration compiles; bounded concurrent stress and quiescent convergence checks pass |
| §B.2 TLA+ model | `tests/correctness/help_update_verify/help_update.tla` and its configs | Sources are present; run `make tlc TLA2TOOLS_JAR=/path/to/tla2tools.jar` for exhaustive model checking |

### Validation boundary

The portable harness establishes source presence, integration, clean
compilation, and a bounded concurrent G2 stress check. It does **not** establish
the paper's hardware-level performance, persistence, crash-consistency, or
multi-host failure-isolation claims. Those require the specified CXL/UB setup,
controller behavior, crash injection, workload datasets, and experiment
topology.

In particular, the supplementary material describes embedding a 16-bit host ID
in optimistic-lock version words and using a controller to clear locks held by
failed hosts. The public BTreeOLC, ART OLC, and Masstree implementations expose
the cache-bypassing lock and flush paths but do not contain that complete
controller-backed recovery protocol. This report therefore does not label
failure isolation for those indexes as verified.

The separately referenced
`/Users/promise/Repository/100-Research/110-PaperWriting/SPOREindex/paper.pdf`
was not available in the Linux workspace used for this audit. Claims appearing
only in that main paper must be checked after the PDF is copied into an
accessible path. This limitation is deliberate and prevents the release record
from claiming evidence that was not inspected.

## Known non-blocking warning

The harness reports four tracked workload source files larger than 10 MiB.
They are benchmark inputs rather than generated binaries. They remain tracked
for reproducibility, but a future release may move them to a versioned dataset
archive with checksums.

## Hardware release checklist

Before attaching a production-readiness or paper-reproduction label, record:

1. exact CPU, CXL/UB device, kernel, firmware, NUMA topology, and toolchain;
2. device-backed runtime and multi-process demo results;
3. YCSB results for every paper-reported index and workload;
4. crash-injection and restart results for durable-linearizability claims;
5. failed-host/controller tests for failure-isolation claims; and
6. the TLA+ tool version, config, explored states, and successful invariant
   output.
