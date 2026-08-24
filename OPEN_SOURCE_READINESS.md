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
- Removed developer-specific Twitter trace paths from the YCSB launcher, added
  explicit dataset validation, made ASLR changes opt-in and restorative, and
  added a small file-backed configuration and smoke workload.

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
YCSB-C `nocc` variant, executes a file-backed NO_CC/BwTree smoke workload, runs
the G2 replicated-pointer stress model, and builds the Chinese Sphinx site with
warnings treated as errors. The smoke build reduces BwTree's mapping-table
width from the paper-scale default of 24 bits to 16 bits so that the same code
path can be exercised within a 256 MiB portable test mapping.

## Paper and supplementary-material traceability

The public main paper used for this audit is
[*Guidelines for Building Indexes on Partially Cache-Coherent CXL Shared
Memory*](https://arxiv.org/abs/2511.06460), arXiv:2511.06460v1. The downloaded
19-page PDF has SHA-256
`132ac31716c7850c7827fd23ce25436ef9a66cc0230bd1b2c441e81f801cbca6`.

The locally available `supplementary-material.pdf` has SHA-256
`bad4f149af2531b2c20136be8d78f2aab77879348ed156e8555ee023d14ae9b9` and is
titled *Indexing Made Consistent and Efficient on Partially Cache Coherent
Shared Memory with SPORE Guidelines*. The table records what can be established
in a portable CI host.

| Main-paper section | Repository evidence | Current validation |
| --- | --- | --- |
| §4 SP conversion guidelines | Cache-bypassing atomics in `shm-lib/include/utils/`; flush/invalidation paths in the index implementations | Runtime and all paper index adapters compile in `nocc` mode |
| §5.4 and §6.1 G2 ClevelHash context replicas | `HelpUpdate`, `meta_replicas`, `load_ptr`, and `cas_ptr` integration | Production integration compiles; bounded G2 stress and convergence checks pass |
| §6.2 G2/G3 BwTree | `OPT_ROOT_READ`, per-thread root metadata, cached pointer fast/slow paths, and invalidation flags in `ds/BwTree/src/bwtree.h` | Optimized path compiles and a reduced-size file-backed NO_CC smoke run completes; no CXL runtime claim is made |
| §6.3 replicated-epoch DGC | `OPT_GC`, per-thread minimum epochs, and delayed reclamation in `ds/BwTree/src/bwtree.h` | Optimized path compiles; hardware crash/reclamation testing remains required |
| §7.1 YCSB setup | 100M A/B/C specs, Zipf exponent 0.99, 24–144 thread sweep, and paper build variants under `tests/YCSB-C/` | Static consistency and clean compilation pass; paper-scale execution requires the documented hardware |
| §7.1 pCAS simulation | 4096 address-hashed queues and configurable simulated non-temporal atomic paths | Implementation compiles; reported latency/bandwidth numbers are not reproduced in CI |
| §7.2 Twitter traces | External trace loader and 47-cluster launcher matrix | Launcher now requires `TRACE_PATH`/`TRACE_NAME`; dataset is not bundled, so results are not reproduced |
| §7.4 Ray/P3-Store | No Ray or P3-Store source is present in this repository | Not reproducible from this repository; must be released separately or scoped out of the artifact claim |

| Supplement section | Repository evidence | Current validation |
| --- | --- | --- |
| §A.1 optimistic locks: BTreeOLC, ART OLC, Masstree | `ds/BTreeOLC/`, `ds/RadixART/OptimisticLockCoupling/`, `ds/Masstree/`; non-temporal version fields and node flush/invalidation paths | All adapters compile in the clean YCSB-C `nocc` build |
| §A.2 lock-free indexes: BwTree and ClevelHash | `ds/BwTree/`, `ds/ClevelHash/`; cache-bypassing atomics and replicated metadata integration | Both adapters compile in the clean `nocc` build |
| §A.3 ROWEX indexes: CLHT and HOT | `ds/CLHT/`, `ds/HOT/`; cache-bypassing key/value/pointer and writer synchronization paths | Both adapters compile in the clean `nocc` build |
| §B G2 replicated pointers | `shm-lib/include/replica_help_update/help_update.h`, `ds/ClevelHash/clevel_hash.hpp`, and `tests/correctness/help_update_verify/` | Production integration compiles; bounded concurrent stress and quiescent convergence checks pass |
| §B.2 TLA+ model | `tests/correctness/help_update_verify/help_update.tla` and its configs | Sources are present; run `make tlc TLA2TOOLS_JAR=/path/to/tla2tools.jar` for exhaustive model checking |

### Validation boundary

The portable harness establishes source presence, integration, clean
compilation, a local NO_CC/BwTree runtime smoke, and a bounded concurrent G2
stress check. It does **not** establish
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

The separately referenced local file
`/Users/promise/Repository/100-Research/110-PaperWriting/SPOREindex/paper.pdf`
was not visible in the Linux workspace. The public arXiv v1 above was inspected
instead. Its byte identity with the unavailable local draft cannot be proven;
copy the local PDF into the retained audit workspace if draft-specific changes
must also be checked. This limitation prevents the release record from claiming
equivalence between two files that were not both available.

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
