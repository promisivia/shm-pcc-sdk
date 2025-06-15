import csv
import statistics
import sys
import numpy as np

def calculate_combined_variance(measurements):
    total_samples = sum(n for _, n, _ in measurements)
    overall_mean = sum(n * mean for _, n, mean in measurements) / total_samples
    
    numerator = sum((n - 1) * variance + n * (mean - overall_mean) ** 2 for variance, n, mean in measurements)
    denominator = total_samples - 1
    
    combined_variance = numerator / denominator
    return combined_variance

def analyze_metric(file_path):
    # 读取文件内容
    data = {}

    with open(file_path, 'r') as file:
        reader = csv.reader(file)
        current_db_count = None
        for row in reader:
            if row[0].startswith('DB Count'):
                current_db_count = row[0].split(': ')[1]
                if current_db_count not in data:
                    data[current_db_count] = []
                data[current_db_count].append({})
            elif row[0] != 'DB':
                db, workload, read_count, update_count, insert_count = row
                if workload not in data[current_db_count][-1]:
                    data[current_db_count][-1][workload] = {
                        'Read Count': [],
                        'Update Count': [],
                        'Insert Count': []
                    }
                data[current_db_count][-1][workload]['Read Count'].append(int(read_count))
                data[current_db_count][-1][workload]['Update Count'].append(int(update_count))
                data[current_db_count][-1][workload]['Insert Count'].append(int(insert_count))

    # 计算每一轮的最大值、最小值和方差
    round_results = {}

    for db_count, rounds in data.items():
        round_results[db_count] = []
        for workloads in rounds:
            round_summary = {}
            for workload, metrics in workloads.items():
                round_summary[workload] = {
                    'Read Count': {
                        'max': max(metrics['Read Count']),
                        'min': min(metrics['Read Count']),
                        'variance': statistics.variance(metrics['Read Count'])
                    },
                    'Update Count': {
                        'max': max(metrics['Update Count']),
                        'min': min(metrics['Update Count']),
                        'variance': statistics.variance(metrics['Update Count'])
                    },
                    'Insert Count': {
                        'max': max(metrics['Insert Count']),
                        'min': min(metrics['Insert Count']),
                        'variance': statistics.variance(metrics['Insert Count'])
                    }
                }
            round_results[db_count].append(round_summary)

    final_results = {}

    for db_count, rounds in round_results.items():
        final_results[db_count] = {}
        for workload in rounds[0].keys():
            final_results[db_count][workload] = {
                'Read Count': {
                    'max': float('-inf'),
                    'min': float('inf'),
                    'variance': 0
                },
                'Update Count': {
                    'max': float('-inf'),
                    'min': float('inf'),
                    'variance': 0
                },
                'Insert Count': {
                    'max': float('-inf'),
                    'min': float('inf'),
                    'variance': 0
                }
            }
            read_counts = []
            update_counts = []
            insert_counts = []
            
            for round_summary in rounds:
                read_counts.append(round_summary[workload]['Read Count'])
                update_counts.append(round_summary[workload]['Update Count'])
                insert_counts.append(round_summary[workload]['Insert Count'])
                
                final_results[db_count][workload]['Read Count']['max'] = max(final_results[db_count][workload]['Read Count']['max'], round_summary[workload]['Read Count']['max'])
                final_results[db_count][workload]['Read Count']['min'] = min(final_results[db_count][workload]['Read Count']['min'], round_summary[workload]['Read Count']['min'])
                
                final_results[db_count][workload]['Update Count']['max'] = max(final_results[db_count][workload]['Update Count']['max'], round_summary[workload]['Update Count']['max'])
                final_results[db_count][workload]['Update Count']['min'] = min(final_results[db_count][workload]['Update Count']['min'], round_summary[workload]['Update Count']['min'])
                
                final_results[db_count][workload]['Insert Count']['max'] = max(final_results[db_count][workload]['Insert Count']['max'], round_summary[workload]['Insert Count']['max'])
                final_results[db_count][workload]['Insert Count']['min'] = min(final_results[db_count][workload]['Insert Count']['min'], round_summary[workload]['Insert Count']['min'])

            final_results[db_count][workload]['Read Count']['variance'] = np.var([x['variance'] for x in read_counts])
            final_results[db_count][workload]['Update Count']['variance'] = np.var([x['variance'] for x in update_counts])
            final_results[db_count][workload]['Insert Count']['variance'] = np.var([x['variance'] for x in insert_counts])
       
    # 打印结果
    for db_count, workloads in final_results.items():
        for workload, metrics in workloads.items():
            for metric, values in metrics.items():
                print(f"DB Count: {db_count}, Workload: {workload}, {metric}: max: {values['max']}, min: {values['min']}, variance: {values['variance']}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python metric_analyze.py <input_path>")
        sys.exit(1)
    
    input_path = sys.argv[1]
    analyze_metric(input_path)
