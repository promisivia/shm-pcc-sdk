echo -e "\033[31mRunning Cache-Coherent BwTree...\033[0m"
./build.sh cc > /dev/null 2>&1
./run_shm_ds.sh -db=bwtree -mode=ycsb-a

echo -e "\033[31mRunning Our Optimized BwTree...\033[0m"
./build.sh nocc > /dev/null 2>&1
./run_shm_ds.sh -db=bwtree -mode=ycsb-a

echo -e "\033[31mRunning not Optimized BwTree...\033[0m"
./build.sh nocc_no_opt > /dev/null 2>&1
./run_shm_ds.sh -db=bwtree -mode=ycsb-a

echo -e "\033[31mRunning Sherman...\033[0m"
./build.sh nocc > /dev/null 2>&1
./run_shm_ds.sh -db=sherman -mode=ycsb-a

echo -e "\033[31mRunning Message-passing BwTree...\033[0m"
./build.sh cc_nocc_mq > /dev/null 2>&1
./run_shm_ds.sh -db=bwtree -mode=ycsb-a
