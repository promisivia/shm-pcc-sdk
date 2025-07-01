PASSWORD=ipads123


create_symlink() {
    local target_name=$1
    ln -sf build_${target_name}/ycsbc_${target_name} ycsbc
}

# 测试ORO在不同的CC配置，以及是否有消息队列的情况下的性能（图11）
task_ycsb_ccconfig_workload_threadcnt_debug() {
    dir=./log/ycsb_ccconfig_workload_threadcnt_debug
    mkdir -p $dir
    configs=("cc_mq")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=server_thread_scale_debug -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

# Fig.13 e-h
# 测试ORO在不同的CC配置，以及是否有消息队列的情况下的性能（图11）
task_ycsb_ccconfig_workload_threadcnt() {
    dir=./log/ycsb_ccconfig_workload_threadcnt
    mkdir -p $dir
    configs=("cc_nocc_mq" "cc" "nocc_no_opt" "nocc")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=server_thread_scale_test -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

#Fig.14 b
# 测试ORO在不同的CC配置，以及是否有消息队列的情况下的性能（图11）
task_real_ccconfig_workload() {
    dir=./log/real_ccconfig_workload
    mkdir -p $dir
    # configs=("nocc" "nocc_no_opt")
    configs=("cc_nocc_mq" "cc" "nocc_part_opt" "nocc_no_opt" "nocc")
    # configs=("cc" "nocc")
    # configs=("nocc")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=run_real_workloads -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

# Fig.15 b
task_bwtree_breakdown() {
    dir=./log/bwtree_breakdown
    mkdir -p $dir
    configs=(
        "nocc_no_opt"
        "nocc_bwtree_part1"
        "nocc_bwtree_part2"
        "nocc" 
        "cc"
    )
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=bwtree_breakdown 2>&1 | tee $dir/${config}.log
    done
}

# Fig.13 a-d
task_clevel_ccconfig_workload_threadcnt() {
    dir=./log/clevel_ccconfig_workload_threadcnt
    mkdir -p $dir
    configs=(
        "cc" 
        "cc_nocc_mq" 
        "nocc" 
        "nocc_no_opt"
        )
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=clevel_server_thread_scale_test 2>&1 | tee $dir/${config}.log
    done
}

# Fig.14 a
# 测试ORO在不同的CC配置，以及是否有消息队列的情况下的性能（图11）
task_clevel_real_ccconfig_workload() {
    dir=./log/real_clevel_ccconfig_workload
    mkdir -p $dir
    # configs=("nocc" "nocc_no_opt")
    configs=("nocc" "nocc_no_opt" "cc" "cc_nocc_mq" )
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=clevel_run_real_workloads_comple 2>&1 | tee $dir/${config}.log
    done
}

# Fig.15 a
task_clevel_breakdown() {
    dir=./log/clevel_breakdown
    mkdir -p $dir
    configs=(
        "nocc_no_opt" 
        "nocc" 
        "cc"
    )
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=clevel_breakdown 2>&1 | tee $dir/${config}.log
    done
}

# Fig.13 sherman
task_sherman_ccconfig_workload_threadcnt() {
    dir=./log/sherman_ccconfig_workload_threadcnt
    mkdir -p $dir
    configs=("nocc")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -db=sherman -mode=server_thread_scale_test 2>&1 | tee $dir/${config}.log
    done
}

task_sherman_indivi() {
    dir=./log/sherman
    mkdir -p $dir
    configs=("nocc")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -db=sherman -mode=sherman 2>&1 | tee $dir/${config}.log
    done
}

task_thread_scale_test() {
    dir=./log/ycsb
    mkdir -p $dir
    configs=("cc" "nocc")
    for config in "${configs[@]}"; do
        ./build.sh ${config}
        ./run_shm_ds.sh -mode=thread_scale_test -db=bwtree 2>&1 | tee $dir/${config}.log
    done
    # configs=("cc_mq" "cc_nocc_mq")
    # for config in "${configs[@]}"; do
    #     ./build.sh ${config}
    #     ./run_shm_ds.sh -mode=client_thread_scale_test -db=bwtree 2>&1 | tee $dir/${config}.log
    # done
}

task_record_scale_test() {
    dir=./log/ycsb
    mkdir -p $dir
    configs=("cc" "nocc")
    for config in "${configs[@]}"; do
        ./build.sh ${config}
        ./run_shm_ds.sh -mode=record_scale_test -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

task3() {
    dir=./log/read-heavy/max64
    mkdir -p $dir
    configs=("cc" "nocc" "nocc_dup_flag")
    for config in "${configs[@]}"; do
        create_symlink ${config}
        ./run_shm_ds.sh -mode=run_test4 -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

task_msg_queue() {
    dir=./log/imbalance
    mkdir -p $dir
    # configs=("cc_mq" )
    configs=("cc_nocc_mq")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=run_msg_queue_test -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

task_dup_flag() {
    dir=./log/dup_flag
    mkdir -p $dir
    configs=("nocc" "nocc_dup_flag")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        create_symlink ${config}
        ./run_shm_ds.sh -mode=test -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

task_super_large_real() {
    dir=./log/real
    mkdir -p $dir
    configs=("cc" "nocc")
    for config in "${configs[@]}"; do
        create_symlink ${config}
        for i in {1..2}; do
            ./run_shm_ds.sh -mode=real-100m -db=bwtree 2>&1 | tee $dir/${config}.log
        done
    done
}

# 检查当分多个数据库时，不同数据库之间的负载会有多不均衡
task_imbalance_multi_db() {
    dir=./log/imbalance_multi_db
    mkdir -p $dir
    configs=("cc_mq")
    for config in "${configs[@]}"; do
        ./build.sh "${config}"
        ./run_shm_ds.sh -mode=run_imbalance_multi_db_test -db=bwtree 2>&1 | tee $dir/${config}.log
    done
}

echo $PASSWORD | sudo -S ./prepare_env.sh

# task_msg_queue
# task_dup_flag
# task_super_large_real
# task_msg_queue
# task_imbalance_multi_db
# task_record_scale_test
task_clevel_ccconfig_workload_threadcnt
# task_clevel_real_ccconfig_workload
# task_ycsb_ccconfig_workload_threadcnt
# task_real_ccconfig_workload
# task_clevel_breakdown
# task_ycsb_ccconfig_workload_threadcnt_debug
# task_sherman_ccconfig_workload_threadcnt
# task_bwtree_breakdown
# task_sherman_indivi
# task_sherman_ccconfig_workload_threadcnt

echo $PASSWORD | sudo -S ./reset_env.sh