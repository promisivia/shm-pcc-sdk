# Contributing to CXL-SDK

Thank you for helping improve CXL-SDK. The project combines original systems
code with adapted research artifacts, so small, well-documented changes are the
easiest to review and reproduce.

## Before you start

- Search existing issues and pull requests before opening a duplicate.
- Use an issue to discuss changes that alter public APIs, persistent layouts,
  shared-memory protocols, or benchmark methodology.
- Do not include confidential traces, credentials, proprietary data, or code
  whose license is incompatible with this repository.

## Development workflow

1. Fork the repository and create a focused branch from `master`.
2. Keep third-party changes separate from CXL-SDK-owned changes when possible.
3. Add or update tests and documentation with the implementation.
4. Run the open-source readiness harness:

   ```bash
   ./tools/opensource-harness/run.sh
   ./tools/opensource-harness/run.sh --full
   ```

5. Describe hardware, kernel, NUMA topology, CXL device, compiler, and workload
   assumptions needed to reproduce experimental results.

## Commit and pull-request guidance

- Write imperative, scoped commit messages such as `fix: handle failed shared
  mapping` or `docs: clarify allocator configuration`.
- Keep generated files and build artifacts out of commits.
- Explain what changed, why it changed, user-visible impact, and validation.
- Mark behavior or performance changes explicitly and include before/after data
  when making performance claims.
- Confirm that new dependencies and copied code include their license and
  attribution information in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## Testing expectations

At minimum, run the harness quick checks. For changes to `shm-lib`, also run the
full harness, which configures and builds the core library in a clean temporary
directory. Hardware-specific tests may be skipped when the required platform is
unavailable, but the pull request must state what was not run and why.

Additional component-specific commands are documented in the
[developer guide](website/zh/docs/developer-guide.md).

## Reporting security issues

Do not open public issues for vulnerabilities. Follow [`SECURITY.md`](SECURITY.md)
instead.

By participating, you agree to follow the
[`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).
