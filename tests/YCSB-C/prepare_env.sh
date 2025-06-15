echo 0 | tee /proc/sys/kernel/numa_balancing >/dev/null
cpupower frequency-set -f 4GHz >/dev/null
echo 3 | tee /proc/sys/vm/drop_caches >/dev/null
echo "prepare env finish"
