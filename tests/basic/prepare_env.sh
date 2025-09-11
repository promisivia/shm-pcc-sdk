echo 0 | sudo tee /proc/sys/kernel/numa_balancing >/dev/null
sudo cpupower frequency-set -f 4GHz >/dev/null
echo 3 | sudo tee /proc/sys/vm/drop_caches >/dev/null

./build/test_basics uncached uncached_mem /dev/uncached_mem_dev para_read_different_mem > uncached_load_diff_addr.log
./build/test_basics uncached uncached_mem /dev/uncached_mem_dev para_read > uncached_load_same_addr.log      
./build/test_basics cached mmap_numa 3 para_read_different_mem  > cached_load_diff_addr.log    
./build/test_basics cached mmap_numa 3 para_read  > cached_load_same_addr.log