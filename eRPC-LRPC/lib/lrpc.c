#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <lrpc/lrpc.h>

static void *map_region(int fd, uint64_t off, size_t len, int prot)
{
	void *p = mmap(NULL, len, prot, MAP_SHARED, fd, (off_t)off);
	return p == MAP_FAILED ? NULL : p;
}

int lrpc_publish(int fd, const void *code, size_t code_size,
		 uint32_t procedure_id, uint64_t epoch)
{
	struct ub_lrpc_set_role role = { .role = UB_LRPC_ROLE_PUBLISHER };
	struct ub_lrpc_publish pub;
	void *dst;

	if (!code || !code_size || code_size > UB_LRPC_CODE_SIZE) {
		errno = EINVAL;
		return -1;
	}
	if (ioctl(fd, UB_LRPC_IOC_SET_ROLE, &role))
		return -1;
	dst = map_region(fd, UB_LRPC_CODE_OFFSET, UB_LRPC_CODE_SIZE,
			 PROT_READ | PROT_WRITE);
	if (!dst)
		return -1;
	memset(dst, 0xcc, UB_LRPC_CODE_SIZE);
	memcpy(dst, code, code_size);
	__sync_synchronize();
	if (munmap(dst, UB_LRPC_CODE_SIZE))
		return -1;

	memset(&pub, 0, sizeof(pub));
	pub.code_epoch = epoch;
	pub.image_size = code_size;
	pub.num_procs = 1;
	pub.proc[0].procedure_id = procedure_id;
	pub.proc[0].code_offset = 0;
	pub.proc[0].code_size = code_size;
	pub.proc[0].astack_size = sizeof(struct lrpc_astack);
	return ioctl(fd, UB_LRPC_IOC_PUBLISH, &pub);
}

int lrpc_bind(struct lrpc_handle *h, const char *device,
	      uint32_t procedure_id, uint64_t expected_epoch)
{
	struct ub_lrpc_set_role role = { .role = UB_LRPC_ROLE_CALLER };
	struct ub_lrpc_bind bind = { .procedure_id = procedure_id,
		.expected_epoch = expected_epoch };
	struct ub_lrpc_info info;
	cpu_set_t set;

	if (!h || !device) {
		errno = EINVAL;
		return -1;
	}
	memset(h, 0, sizeof(*h));
	h->fd = open(device, O_RDWR | O_CLOEXEC);
	if (h->fd < 0)
		return -1;
	if (ioctl(h->fd, UB_LRPC_IOC_SET_ROLE, &role) ||
	    ioctl(h->fd, UB_LRPC_IOC_BIND, &bind))
		goto fail;
	memset(&info, 0, sizeof(info));
	if (ioctl(h->fd, UB_LRPC_IOC_INFO, &info) || !info.shadow_ready ||
	    info.shadow_cpu < 0) {
		errno = ENOTCONN;
		goto fail;
	}
	CPU_ZERO(&set);
	CPU_SET(info.shadow_cpu, &set);
	if (sched_setaffinity(0, sizeof(set), &set))
		goto fail;
	h->astack_map = map_region(h->fd, UB_LRPC_ASTACK_OFFSET,
				  UB_LRPC_ASTACK_SIZE, PROT_READ | PROT_WRITE);
	if (!h->astack_map)
		goto fail;
	h->epoch = bind.expected_epoch;
	return 0;
fail:
	lrpc_close(h);
	return -1;
}

int lrpc_invoke(struct lrpc_handle *h, struct lrpc_astack *call)
{
	struct lrpc_astack *shared = NULL;
	struct ub_lrpc_handoff handoff;

	if (!h || !call || !h->astack_map) {
		errno = EINVAL;
		return -1;
	}
	if (call->request_size > LRPC_PAYLOAD_MAX) {
		errno = EMSGSIZE;
		return -1;
	}
	shared = h->astack_map;
	memcpy(shared, call, sizeof(*shared));
	shared->caller_cpu_before = (uint64_t)sched_getcpu();
	shared->caller_cpu_after = UINT64_MAX;
	shared->caller_pid = (uint64_t)getpid();
	__sync_synchronize();
	memset(&handoff, 0, sizeof(handoff));
	if (ioctl(h->fd, UB_LRPC_IOC_CALL, &handoff))
		return -1;
	__sync_synchronize();
	shared->caller_cpu_after = (uint64_t)sched_getcpu();
	shared->call_enter_ns = handoff.call_enter_ns;
	shared->shadow_dispatch_ns = handoff.shadow_dispatch_ns;
	shared->shadow_return_ns = handoff.shadow_return_ns;
	shared->caller_resume_ns = handoff.caller_resume_ns;
	memcpy(call, shared, sizeof(*call));
	return handoff.result;
}

int lrpc_shadow_run(const char *device, uint32_t procedure_id,
		    uint64_t expected_epoch)
{
	struct ub_lrpc_set_role role = { .role = UB_LRPC_ROLE_SHADOW };
	struct ub_lrpc_bind bind = { .procedure_id = procedure_id,
		.expected_epoch = expected_epoch };
	struct ub_lrpc_info info;
	struct ub_lrpc_handoff handoff;
	struct lrpc_astack *shared = NULL;
	void *code = NULL, *data = NULL, *estack = MAP_FAILED;
	size_t estack_size = 64 * 1024;
	cpu_set_t set;
	const char *cpu_env;
	int cpu = 1, fd = -1, ret = -1;

	if (!device) {
		errno = EINVAL;
		return -1;
	}
	cpu_env = getenv("LRPC_SHADOW_CPU");
	if (cpu_env)
		cpu = atoi(cpu_env);
	CPU_ZERO(&set);
	CPU_SET(cpu, &set);
	if (sched_setaffinity(0, sizeof(set), &set))
		return -1;
	fd = open(device, O_RDWR | O_CLOEXEC);
	if (fd < 0 || ioctl(fd, UB_LRPC_IOC_SET_ROLE, &role) ||
	    ioctl(fd, UB_LRPC_IOC_BIND, &bind))
		goto out;
	memset(&info, 0, sizeof(info));
	if (ioctl(fd, UB_LRPC_IOC_INFO, &info))
		goto out;
	code = map_region(fd, UB_LRPC_CODE_OFFSET, UB_LRPC_CODE_SIZE,
			  PROT_READ | PROT_EXEC);
	shared = map_region(fd, UB_LRPC_ASTACK_OFFSET, UB_LRPC_ASTACK_SIZE,
			    PROT_READ | PROT_WRITE);
	data = map_region(fd, UB_LRPC_DATA_OFFSET, 4096, PROT_READ | PROT_WRITE);
	estack = mmap(NULL, estack_size, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (!code || !shared || !data || estack == MAP_FAILED)
		goto out;
	if (ioctl(fd, UB_LRPC_IOC_REGISTER_SHADOW))
		goto out;
	printf("LRPC_SHADOW_REGISTERED pid=%d cpu=%d mm=separate\n",
	       getpid(), sched_getcpu());
	fflush(stdout);
	for (;;) {
		memset(&handoff, 0, sizeof(handoff));
		if (ioctl(fd, UB_LRPC_IOC_WAIT_CALL, &handoff))
			goto out;
		shared->service_data = (uint64_t)(uintptr_t)data;
		shared->shadow_pid = (uint64_t)getpid();
		__sync_synchronize();
		handoff.result = (int)lrpc_call_on_stack(
			(uint8_t *)code + bind.entry_offset, shared,
			(uint8_t *)estack + estack_size);
		__sync_synchronize();
		if (ioctl(fd, UB_LRPC_IOC_RETURN, &handoff))
			goto out;
	}
out:
	if (estack != MAP_FAILED)
		munmap(estack, estack_size);
	if (data)
		munmap(data, 4096);
	if (shared)
		munmap(shared, UB_LRPC_ASTACK_SIZE);
	if (code)
		munmap(code, UB_LRPC_CODE_SIZE);
	if (fd >= 0)
		close(fd);
	return ret;
}

void lrpc_close(struct lrpc_handle *h)
{
	if (!h)
		return;
	if (h->astack_map)
		munmap(h->astack_map, UB_LRPC_ASTACK_SIZE);
	if (h->fd >= 0)
		close(h->fd);
	memset(h, 0, sizeof(*h));
	h->fd = -1;
}
