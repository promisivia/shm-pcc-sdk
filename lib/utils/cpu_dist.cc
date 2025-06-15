#include "utils/cpu_dist.h"

#include <pthread.h>

#include <cassert>
#include <iostream>

int get_first_cpu_of_numa_node(int node) {
  struct bitmask *cpumask = numa_allocate_cpumask();
  if (numa_node_to_cpus(node, cpumask) != 0) {
    std::cerr << "Failed to get CPUs for NUMA node " << node << std::endl;
    numa_free_cpumask(cpumask);
    return -1;
  }

  for (ulong i = 0; i < cpumask->size; ++i) {
    if (numa_bitmask_isbitset(cpumask, i)) {
      numa_free_cpumask(cpumask);
      return i;
    }
  }

  numa_free_cpumask(cpumask);
  return -1;  // No CPU found for the given NUMA node
}

int get_cpu_nr_numa(int node) {
  int cpu_nr = 0;
  bitmask *cpumask = numa_allocate_cpumask();
  numa_node_to_cpus(0, cpumask);

  for (ulong cpu = 0; cpu < cpumask->size; ++cpu) {
    if (numa_bitmask_isbitset(cpumask, cpu)) {
      cpu_nr++;
    }
  }
  numa_free_cpumask(cpumask);
  return cpu_nr;
}

static unsigned int last_cpu = 0;

void set_last_cpu(unsigned int cpu) { last_cpu = cpu; }

int get_available_cpu_server() {
  unsigned int cpu_count = numa_num_configured_cpus();
  unsigned int next_cpu = last_cpu + 1;
  assert(next_cpu < cpu_count);
  last_cpu = next_cpu;
  return next_cpu;
}

int set_pthread_affinity_attr(int cpu, pthread_attr_t &attr) {
  pthread_attr_init(&attr);

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  int rc = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    std::cerr << "Error setting thread affinity to CPU " << cpu << std::endl;
    return rc;
  }
  return 0;
}

int set_pthread_affinity(int cpu) {
  pthread_t tid = pthread_self();

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  int rc = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
    std::cerr << "Error setting thread affinity for thread " << tid
              << " to CPU " << cpu << std::endl;
    return rc;
  }
  return 0;
}

CpuAllocator cpu_allocator;
