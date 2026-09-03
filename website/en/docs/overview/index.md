# Overview

CXL-SDK reduces the work required to prototype shared-memory and CXL/UB
systems. It packages common infrastructure into components that applications
can combine instead of rebuilding memory mapping, shared objects, and
concurrent containers from scratch.

## What the SDK provides

- **Shared-memory runtime:** memory mapping, object lifetime, allocators, and
  non-temporal pointers.
- **Concurrent data structures:** integrations for BwTree, Masstree, CLHT, and
  a common evaluation path.
- **Communication:** shared-memory queues, RPC, and multi-process or
  multi-machine coordination.
- **Applications and evaluation:** YCSB-C and repository applications for
  correctness and performance experiments.

## Install and use

Follow the {doc}`../user-guide` to install dependencies, build the project, and
run a first workload. Runtime settings are collected in
{doc}`../environment-variables`.

## Roadmap

See the {doc}`../roadmap` for current progress and planned work.
