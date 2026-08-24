# Technical Direction

## Motivation

Emerging CXL and Unified Buffer platforms make it possible for applications on
different machines to access a shared memory region directly.  This can reduce
copies and avoid parts of the coordination overhead found in conventional
network messaging.  The SDK provides the common runtime and data-structure
building blocks required to evaluate those systems without reimplementing the
same infrastructure in every application.

Representative use cases include transaction processing, zero-copy
cross-machine communication, graph processing, and distributed analytics.

## Design approach

The SDK treats shared data structures as the primary programming abstraction.
Applications use containers such as maps, trees, vectors, and hash tables to
store and exchange state; the runtime then provides the mapping, allocation,
coordination, and communication mechanisms underneath them.

This design has three practical benefits:

1. Data-structure implementations can centralize correctness and performance
   decisions for partially cache-coherent shared-memory platforms.
2. Roots and reachability information offer a foundation for detecting
   allocations that are no longer live after a partial failure, which supports
   future reclamation work.
3. Runtime components such as RPC can make better decisions when the shared
   data and its access semantics are expressed through SDK interfaces.

## Current focus

The ongoing work is organized around four areas:

- **Synchronization:** lock-based, lock-free, and transactional-memory
  mechanisms appropriate for the target hardware.
- **Data structures:** shared-memory hash tables and trees, followed by more
  standard container-style interfaces.
- **System components:** object storage and RPC integrations that validate the
  runtime with realistic services such as microservices and Redis-like
  workloads.
- **Memory management:** allocator development, liveness tracking, and
  reclamation strategies for shared CXL memory.

The roadmap is intentionally incremental: interfaces are evaluated with real
workloads before they become stable SDK contracts.
