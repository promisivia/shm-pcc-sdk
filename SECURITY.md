# Security Policy

## Supported versions

CXL-SDK is currently developed on the `master` branch and has not yet published
a stable release series. Security fixes are applied to `master`; older commits
and experimental branches are not supported.

## Reporting a vulnerability

Please do not disclose suspected vulnerabilities in a public issue, discussion,
or pull request.

Use GitHub's **Private vulnerability reporting** or a private security advisory
for this repository. Include:

- affected commit or component;
- impact and threat model;
- reproduction steps or a minimal proof of concept;
- required hardware, privileges, and configuration; and
- any suggested mitigation.

Maintainers will acknowledge a complete report as soon as practical, assess its
scope, coordinate a fix, and agree on disclosure timing with the reporter.

## Security model and limitations

CXL-SDK is systems-research software. Several tools intentionally use shared
memory, fixed virtual addresses, privileged device access, SSH-based orchestration,
or changes to kernel runtime settings. These mechanisms do not create a security
boundary between mutually untrusted participants. Review scripts and
configuration before running them, use isolated test machines, and never place
credentials in repository configuration files.
