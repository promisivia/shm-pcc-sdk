import subprocess
import re
import datetime
import time
import os
import csv

def parse_perf_output(output_str, event_name):
    """
    解析 perf stat 输出以查找特定事件的计数值。
    示例行: "     1,234,567      unc_cha_tor_occupancy.ddr"
    """
    for line in output_str.splitlines():
        if event_name in line:
            parts = line.strip().split()
            if parts:
                count_str = parts[0]
                # 处理 "<not counted>" 或 "<not supported>"
                if count_str.lower() == "<not" and len(parts) > 1 and \
                   ("counted" in parts[1].lower() or "supported" in parts[1].lower()):
                    status_msg = f"{parts[0]} {parts[1]}"
                    print(f"警告: 事件 {event_name} 报告为 '{status_msg}'")
                    return None
                try:
                    return int(count_str.replace(',', ''))
                except ValueError:
                    print(f"警告: 无法从 '{count_str}' 解析事件 {event_name} 的计数。问题行: '{line.strip()}'")
                    return None
            else:
                print(f"警告: 找到事件 {event_name} 的行，但无法解析计数。行: '{line.strip()}'")
                return None
    return None # 未在任何行中找到事件名称

def get_cpu_frequency():
    """获取当前CPU频率 (第一个核心的)"""
    try:
        with open("/proc/cpuinfo", "r") as f:
            for line in f:
                if "cpu MHz" in line:
                    # 示例行: "cpu MHz         : 2900.000"
                    parts = line.split(":")
                    if len(parts) > 1:
                        print("当前CPU频率："+ parts[1])
                        return float(parts[1].strip())*1e6
    except FileNotFoundError:
        print("警告: /proc/cpuinfo 未找到，无法获取CPU频率。")
        return None
    except Exception as e:
        print(f"警告: 获取CPU频率时出错: {e}")
        return None
    return None # 未找到CPU频率信息

def main():
    perf_events = "unc_cha_tor_occupancy.all,unc_cha_tor_inserts.all"
    perf_command = [
        "sudo", "perf", "stat", "-e", perf_events,
        "-a", "sleep", "1"
    ]

    # CSV 和符号链接设置
    try:
        script_path = os.path.abspath(__file__)
        script_dir = os.path.dirname(script_path) + "/tor_perf"
        if not os.path.exists(script_dir):
            os.mkdir(script_dir)
    except NameError: # 如果在交互式解释器中运行（__file__ 未定义）
        exit(1)


    start_time_dt = datetime.datetime.now()
    csv_filename_base = f"perf_data_{start_time_dt.strftime('%Y%m%d_%H%M%S')}.csv"
    full_csv_path = os.path.join(script_dir, csv_filename_base)

    symlink_name_base = "latest_perf_data.csv"
    full_symlink_path = os.path.join(script_dir, symlink_name_base)

    # 创建/更新符号链接
    # target 对于符号链接应该是相对于符号链接目录的名称，或者绝对路径
    if os.path.lexists(full_symlink_path):
        os.remove(full_symlink_path)
    try:
        os.symlink(csv_filename_base, full_symlink_path) # 目标，链接名
        print(f"符号链接 '{full_symlink_path}' 已创建，指向 '{csv_filename_base}'")
    except OSError as e:
        print(f"错误: 创建符号链接失败: {e}")
        # 根据操作系统和权限，可能无法创建符号链接。脚本仍可继续。

    print(f"开始收集性能数据并计算DDR请求延迟...")
    print(f"数据将记录到: {full_csv_path}")
    
    csv_file = None # 初始化以用于 finally 块

    try:
        with open(full_csv_path, 'w', newline='') as f_csv:
            csv_file = f_csv # 赋值给外部变量以便 finally 块可以访问
            csv_writer = csv.writer(f_csv)
            csv_writer.writerow(["Timestamp", "Occupancy_Count", "Inserts_Count", "Average_Latency_s"])
            f_csv.flush()

            while True:
                try:
                    # 运行 perf 命令
                    process = subprocess.run(
                        perf_command,
                        capture_output=True,
                        text=True,
                        check=False # 手动检查，因为 perf stat 将输出发送到 stderr
                    )
                    
                    # perf stat 输出在 stderr 上
                    perf_output_stderr = process.stderr 
                    # 有些版本的 perf 可能在 stdout 上输出摘要，但计数通常在 stderr
                    # perf_output_stdout = process.stdout 
                    # perf_output = perf_output_stderr + "\n" + perf_output_stdout # 合并以防万一

                    # 解析计数值
                    occupancy_count = parse_perf_output(perf_output_stderr, "unc_cha_tor_occupancy.all")
                    inserts_count = parse_perf_output(perf_output_stderr, "unc_cha_tor_inserts.all")
                    cpu_freq = get_cpu_frequency() # 获取CPU频率

                    current_timestamp_iso = datetime.datetime.now().isoformat()

                    if occupancy_count is None or inserts_count is None:
                        message = f"{current_timestamp_iso} - 无法获取性能事件计数，请检查事件名称和权限。"
                        print(message)
                        csv_writer.writerow([current_timestamp_iso, "Error", "Error", "Error"])
                    elif inserts_count == 0:
                        message = f"{current_timestamp_iso} - 在此间隔内未检测到DDR请求插入。"
                        print(message)
                        csv_writer.writerow([current_timestamp_iso, occupancy_count, inserts_count, "N/A (no inserts)"])
                    else:
                        latency = occupancy_count / inserts_count * 1e9 / cpu_freq
                        message = f"{current_timestamp_iso} - 平均延迟：{latency:.6f} ns"
                        print(message)
                        csv_writer.writerow([current_timestamp_iso, occupancy_count, inserts_count, f"{latency:.6f}"])
                    
                    f_csv.flush() # 确保数据写入磁盘

                except FileNotFoundError:
                    print("错误: 'perf' 命令未找到。请确保已安装perf工具并将其添加到PATH中。")
                    csv_writer.writerow([datetime.datetime.now().isoformat(), "FATAL_ERROR", "perf not found", "FATAL_ERROR"])
                    f_csv.flush()
                    break # 如果 perf 未找到，则退出循环
                except Exception as e:
                    error_timestamp = datetime.datetime.now().isoformat()
                    print(f"{error_timestamp} - 发生内部错误: {e}")
                    try:
                        csv_writer.writerow([error_timestamp, "INTERNAL_ERROR", str(e), "INTERNAL_ERROR"])
                        f_csv.flush()
                    except Exception as csv_e:
                        print(f"写入CSV时发生额外错误: {csv_e}")
                
                time.sleep(1) # 等待1秒

    except KeyboardInterrupt:
        print("\n脚本被用户中断。正在关闭...")
    except IOError as e:
        print(f"发生文件操作错误 (例如无法写入CSV): {e}")
    except Exception as e:
        print(f"发生未预料的严重错误: {e}")
    finally:
        if csv_file and not csv_file.closed:
            csv_file.close()
            print("CSV文件已关闭。")
        else:
            print("CSV文件未打开或已关闭。")


if __name__ == "__main__":
    main()
