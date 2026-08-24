chmod +x ./ycsbc
# LD_LIBRARY_PATH=/home/pcc-shm/shared_libs \
#     ./ycsbc -db "bwtree" -machinenum "2" -follower_list "90.91.106.251" \
    # -client_threads "640" -server_threads "640" -dbnum "1" -P "workloads/workloadc_zipfian_10m.spec" -C config.ini
LD_LIBRARY_PATH=/home/pcc-shm/shared_libs \
    ./ycsbc -db "bwtree" -machinenum "1" -follower_list "90.91.106.251" \
    -client_threads "320" -server_threads "320" -dbnum "1" -P "workloads/workloadc_zipfian_10m.spec" -C config.ini
# LD_LIBRARY_PATH=/home/pcc-shm/shared_libs \
#     ./ycsbc -db "bwtree" -machinenum "2" -follower_list "90.91.106.250" \
#     -client_threads "2" -server_threads "2" -dbnum "1" -P "workloads/workloada.spec" -C config.ini