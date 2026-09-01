# eRPC-LRPC: executable ivshmem LRPC prototype

This directory is a Linux/QEMU proof of concept for the mechanism described in
`UB跨机LRPC专利交底-Typst.pdf`.  Guest B publishes a position-independent RX
service image and exported service data into an ivshmem BAR. In guest A, an
independent passive shadow process registers with the kernel driver and maps
the code RX, service data RW, shared A-stack, and a private E-stack. The caller
maps only the A-stack. `CALL` blocks the caller and wakes the CPU-pinned shadow;
the normal Linux scheduler switches to the shadow's distinct `mm_struct`/CR3.
`RETURN` blocks the shadow again and wakes the caller. There is no request
queue and no guest-B CPU on the invocation datapath.

The Linux driver enforces role-based mappings, W^X, distinct caller/shadow
address spaces, same-CPU handoff, and procedure validation. The pinned upstream eRPC tree is built with an LRPC
datapath: ordinary `Rpc::enqueue_request` invokes `lrpc_invoke()` locally and
feeds a standard response completion back to eRPC. UDP remains only for eRPC's
session-management handshake.

## Security and emulation boundary

QEMU `ivshmem-plain` is a functional stand-in for a UB coherent memory window;
it does not model UB latency, cache invalidation, poison, or fabric faults.  The
prototype uses a separate Linux process as a ChCore-style passive shadow
thread, so caller and service execution have distinct `mm_struct`s. This is a
scheduler-mediated handoff rather than a new in-kernel architecture-specific
context-switch primitive. Arbitrary normal C/C++ handlers cannot be
copied as code blobs because relocations and process-local pointers would be
invalid; published handlers must use the fixed PIC ABI.

## Build

```sh
./scripts/fetch-deps.sh       # only needed in a fresh checkout
./scripts/build.sh
./scripts/build-kernel-module.sh
./scripts/build-vm.sh
ACCEL=kvm ./scripts/run-qemu.sh  # hardware virtualization (recommended)
# ./scripts/run-qemu.sh          # TCG fallback
```

The QEMU scripts build a small Linux 6.6 LTS kernel/initramfs, attach one 4 MiB
ivshmem backing file to both guests, and check separate caller/shadow PIDs,
denied caller code/data mappings, a result that consumes exported B data, and
equal CPU IDs before/during/after the service call. See `docs/design.md` for the ABI
and remaining work toward a production eRPC backend.
