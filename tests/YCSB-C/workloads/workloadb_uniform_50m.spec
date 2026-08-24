# Yahoo! Cloud Serving Benchmark
# Workload B: 95% read, 5% update, uniform distribution

recordcount=50000000
operationcount=50000000
workload=com.yahoo.ycsb.workloads.CoreWorkload

readallfields=true
readproportion=0.95
updateproportion=0.05
scanproportion=0
insertproportion=0
requestdistribution=uniform
