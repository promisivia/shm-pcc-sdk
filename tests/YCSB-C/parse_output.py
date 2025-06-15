#!/bin/python3

import re
import sys
from collections import defaultdict

def extract_info(file_path, throughput_output_path, db_metrics_output_path):
    with open(file_path, 'r') as file:
        content = file.read()

    pattern = re.compile(
        r'# Transaction throughput \(KTPS\)\n(\w+)\s+workloads/(\w+)\.spec\s+(\d+)\s+(\d+)\s+([\d.]+)\n((?:DB \d+: Read Count: \d+ Update Count: \d+ Insert Count: \d+\n)+)'
    )
    
    matches = pattern.findall(content)

    throughput_data = defaultdict(lambda: [0, 0])  # Dictionary to store sum and count
    db_data = []
    db_metrics_matches = None

    for match in matches:
        db_type, workload, cthreads, sthreads, throughput, db_metrics = match
        workload_char = workload[-1].upper()  # Convert workload to single character
        key = (workload_char, cthreads, sthreads)
        throughput_data[key][0] += float(throughput)  # Sum of throughputs
        throughput_data[key][1] += 1  # Count of occurrences

        db_metrics_pattern = re.compile(r'DB (\d+): Read Count: (\d+) Update Count: (\d+) Insert Count: (\d+)')
        db_metrics_matches = db_metrics_pattern.findall(db_metrics)
        for db_id, read_count, update_count, insert_count in db_metrics_matches:
            db_data.append((db_id, workload_char, read_count, update_count, insert_count))

    with open(throughput_output_path, 'a') as throughput_file:
        # if len(db_metrics_matches) == 1:
        #     throughput_file.write("Record,Workload,Client Thread,Server Thread,DB,Throughput\n")
        for key, value in throughput_data.items():
            workload_char, cthreads, sthreads = key
            avg_throughput = value[0] / value[1]  # Calculate average
            throughput_file.write(f"{workload_char},{cthreads},{sthreads},{len(db_metrics_matches)},{avg_throughput:.2f}\n")

    # with open(db_metrics_output_path, 'a') as db_file:
    #     db_file.write("DB,Workload,Read Count,Update Count,Insert Count\n")
    #     for db_id, workload_char, read_count, update_count, insert_count in db_data:
    #         db_file.write(f"{db_id},{workload_char},{read_count},{update_count},{insert_count}\n")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python parse_output.py <input_path> <throughput_output_path> <db_metrics_output_path>")
        sys.exit(1)

    input_path = sys.argv[1]
    throughput_output_path = sys.argv[2]
    db_metrics_output_path = sys.argv[3]
    extract_info(input_path, throughput_output_path, db_metrics_output_path)