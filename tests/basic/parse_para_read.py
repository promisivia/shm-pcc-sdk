import re
import matplotlib.pyplot as plt
import numpy as np

mode = 'uncached_local' # 'uncached', 'uncached-fsync', 'uncached-fsync-mfence'
# mode = 'uncached_remote' # 'uncached', 'uncached-fsync', 'uncached-fsync-mfence'

log_file = './'+mode+'.log'

thread_times = {}
current_threads = None
start_parsing = False

for line in open(log_file, 'r'):
    match_thread = re.match(r'Average read time for (\d+) threads', line)
    if match_thread:
        # print(match_thread.group(1))
        current_threads = int(match_thread.group(1))
        thread_times[current_threads] = []
        start_parsing = True
        continue
    if start_parsing is True:
        # Average read time: 1093.43ns or Average read time: 41879ns
        match_time = re.match(r'Average time: ([\d.]+)ns', line)
        if match_time:
            thread_times[current_threads].append(float(match_time.group(1)))
        else:
            start_parsing = False

thread_nums = list(thread_times.keys())
data = [thread_times[num] for num in thread_nums]

plt.figure(figsize=(8, 6))
# fontsize
plt.rc('font', size=12)
# The 'labels' parameter of boxplot() has been renamed 'tick_labels' since Matplotlib 3.9; support for the old name will be dropped in 3.11.
plt.boxplot(data, tick_labels=thread_nums)
# plt.boxplot(data, labels=thread_nums)
# show average value on top of each box
for i, num in enumerate(thread_nums):
    y = np.mean(data[i])
    plt.text(i + 1, y, f'{y:.2f}', ha='center', va='bottom', fontsize=12)
plt.title('Average Read Time by Thread Count ({})'.format(mode))
plt.xlabel('Number of Threads')
plt.ylabel('Average Read Time (ns)')
plt.grid(axis='y')
plt.tight_layout()
plt.savefig('{}.png'.format(mode))  
