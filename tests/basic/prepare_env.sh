echo 0 | sudo tee /proc/sys/kernel/numa_balancing >/dev/null
sudo cpupower frequency-set -f 4GHz >/dev/null
echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null
