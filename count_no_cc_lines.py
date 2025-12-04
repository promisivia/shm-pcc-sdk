#!/usr/bin/env python3
"""
统计 ds/ 目录下每个子文件夹中，在 #ifdef NO_CC 或 #if defined(NO_CC) 到 #else 或 #endif 之间的代码行数
同时统计真正有用的 C++/HPP 文件的代码行数（排除空行和注释）
"""

import os
import re
from collections import defaultdict
from pathlib import Path

# 源代码文件扩展名
SOURCE_EXTENSIONS = {'.cpp', '.cc', '.c', '.h', '.hpp', '.hxx', '.cxx'}

# C++/HPP 文件扩展名（用于统计总行数）
CPP_EXTENSIONS = {'.cpp', '.cc', '.c', '.h', '.hpp', '.hxx', '.cxx'}

# 需要排除的目录名关键词
EXCLUDE_DIR_KEYWORDS = {'test', 'tests', 'example', 'examples', 'demo', 'demos', 
                        'sample', 'samples', 'benchmark', 'benchmarks', 'third-party',
                        'third_party', '3rdparty', '3rd-party'}

# 需要排除的文件名关键词
EXCLUDE_FILE_KEYWORDS = {'test', 'example', 'demo', 'sample', 'benchmark', 
                         'crash_test', 'stress_test', 'unit_test'}

def is_no_cc_condition(condition_text):
    """
    检查条件文本是否包含 NO_CC 相关的条件
    支持：
    - #ifdef NO_CC
    - #if defined(NO_CC)
    - #if defined NO_CC
    - #elif defined(NO_CC)
    - #elif defined NO_CC
    - #if !defined(NO_CC)  (也统计)
    - #if (defined(NO_CC))
    - 多行条件（使用反斜杠续行）
    等等
    
    注意：如果条件中包含任何以 OPT 开头的宏定义（如 defined(OPT_GC)），则不算作 NO_CC 块
    """
    # 移除所有空白字符，便于匹配
    normalized = ' '.join(condition_text.split())
    
    # 检查 #ifdef NO_CC 或 #elif NO_CC（简单的 #ifdef/#elif 不涉及 OPT）
    if re.search(r'#(?:ifdef|elif)\s+NO_CC\b', normalized):
        return True
    
    # 检查 #if 或 #elif 开头，且包含 defined(NO_CC) 或 defined NO_CC
    if re.search(r'#(?:if|elif)\b', normalized):
        # 首先检查是否包含 NO_CC
        has_no_cc = re.search(r'defined\s*\(?\s*NO_CC\s*\)?', normalized, re.IGNORECASE)
        
        if has_no_cc:
            # 检查是否包含任何以 OPT 开头的宏定义
            # 匹配 defined(OPT...) 或 defined OPT...
            has_opt = re.search(r'defined\s*\(?\s*OPT\w+', normalized, re.IGNORECASE)
            
            # 如果包含 OPT 开头的宏，不算作 NO_CC 块
            if has_opt:
                return False
            
            # 包含 NO_CC 且不包含 OPT 开头的宏，算作 NO_CC 块
            return True
    
    return False

def count_no_cc_lines_in_file(file_path):
    """
    统计单个文件中 #ifdef NO_CC 或 #if defined(NO_CC) 到 #else 或 #endif 之间的行数
    支持多行条件编译指令（使用反斜杠续行）
    返回: (总行数, 块数)
    """
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Warning: Cannot read {file_path}: {e}")
        return 0, 0
    
    total_lines = 0
    block_count = 0
    in_no_cc_block = False
    ifdef_line = -1
    
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        line_num = i + 1
        
        # 检查是否是 #if、#ifdef 或 #elif 开头（可能是多行的）
        if stripped.startswith('#if') or stripped.startswith('#elif'):
            # 保存原始的指令类型（用于后续判断）
            is_elif = stripped.startswith('#elif')
            
            # 收集完整的条件编译指令（可能跨多行）
            condition_lines = [line]
            condition_line_nums = [line_num]
            j = i + 1
            current_stripped = stripped
            
            # 如果当前行以反斜杠结尾，继续读取下一行
            while j < len(lines) and current_stripped.endswith('\\'):
                condition_lines.append(lines[j])
                condition_line_nums.append(j + 1)
                current_stripped = lines[j].strip()
                j += 1
            
            # 合并所有条件行
            full_condition = ''.join(condition_lines)
            
            # 检查是否包含 NO_CC
            if is_no_cc_condition(full_condition):
                # 如果之前不在 NO_CC 块中，开始新的块
                if not in_no_cc_block:
                    in_no_cc_block = True
                    ifdef_line = condition_line_nums[0]  # 使用第一行的行号
                    block_count += 1
                # 如果已经在 NO_CC 块中（可能是从 #if 切换到 #elif），更新起始行
                else:
                    # 统计之前块的行数（从上一个起始行到当前 #elif 之前）
                    total_lines += (line_num - ifdef_line - 1)
                    ifdef_line = condition_line_nums[0]  # 更新起始行
                i = j  # 跳过已处理的行
                continue
            else:
                # 如果不是 NO_CC 条件，且是 #elif，如果之前在 NO_CC 块中，结束该块
                if is_elif and in_no_cc_block:
                    # 统计从起始行到当前 #elif 之前的行数
                    total_lines += (line_num - ifdef_line - 1)
                    in_no_cc_block = False
            
            i = j
            continue
        
        # 如果在 NO_CC 块中
        if in_no_cc_block:
            # 检查 #else
            if stripped.startswith('#else'):
                # 统计从 #ifdef 到 #else 之间的行数（不包括这两行）
                total_lines += (line_num - ifdef_line - 1)
                in_no_cc_block = False
                i += 1
                continue
            
            # 检查 #endif
            if stripped.startswith('#endif'):
                # 统计从 #ifdef 到 #endif 之间的行数（不包括这两行）
                total_lines += (line_num - ifdef_line - 1)
                in_no_cc_block = False
                i += 1
                continue
        
        i += 1
    
    return total_lines, block_count

def is_useful_file(file_path):
    """
    判断文件是否是真正有用的源代码文件（排除测试、示例等文件）
    """
    file_path_str = str(file_path).lower()
    file_name = file_path.name.lower()
    parent_dirs = [p.name.lower() for p in file_path.parents]
    
    # 检查目录名
    for dir_name in parent_dirs:
        for keyword in EXCLUDE_DIR_KEYWORDS:
            if keyword in dir_name:
                return False
    
    # 检查文件名
    for keyword in EXCLUDE_FILE_KEYWORDS:
        if keyword in file_name:
            return False
    
    return True

def count_code_lines_in_file(file_path):
    """
    统计文件中有效的代码行数（排除空行和注释）
    返回: 代码行数
    """
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Warning: Cannot read {file_path}: {e}")
        return 0
    
    code_lines = 0
    in_multiline_comment = False
    
    for line in lines:
        stripped = line.strip()
        
        # 跳过空行
        if not stripped:
            continue
        
        # 处理多行注释
        if in_multiline_comment:
            # 查找注释结束
            if '*/' in stripped:
                # 提取注释结束后的内容
                end_idx = stripped.find('*/') + 2
                stripped = stripped[end_idx:].strip()
                in_multiline_comment = False
            else:
                # 整行都在注释中
                continue
        
        # 处理单行注释和多行注释开始
        while stripped:
            # 查找单行注释
            single_comment_idx = stripped.find('//')
            # 查找多行注释开始
            multi_start_idx = stripped.find('/*')
            
            # 确定哪个先出现
            comment_start = -1
            comment_type = None
            
            if single_comment_idx >= 0 and multi_start_idx >= 0:
                if single_comment_idx < multi_start_idx:
                    comment_start = single_comment_idx
                    comment_type = 'single'
                else:
                    comment_start = multi_start_idx
                    comment_type = 'multi'
            elif single_comment_idx >= 0:
                comment_start = single_comment_idx
                comment_type = 'single'
            elif multi_start_idx >= 0:
                comment_start = multi_start_idx
                comment_type = 'multi'
            
            if comment_start >= 0:
                # 检查注释是否在字符串中（简单检查，不处理转义）
                # 查找注释前的引号
                before_comment = stripped[:comment_start]
                single_quotes = before_comment.count("'") - before_comment.count("\\'")
                double_quotes = before_comment.count('"') - before_comment.count('\\"')
                
                # 如果引号数量是奇数，说明注释在字符串中，跳过
                if single_quotes % 2 == 1 or double_quotes % 2 == 1:
                    # 注释在字符串中，保留整行
                    code_lines += 1
                    break
                
                # 注释不在字符串中
                if comment_type == 'single':
                    # 单行注释，保留注释前的部分
                    before_comment = stripped[:comment_start].strip()
                    if before_comment:
                        code_lines += 1
                    break
                else:
                    # 多行注释开始
                    before_comment = stripped[:comment_start].strip()
                    if before_comment:
                        code_lines += 1
                    
                    # 查找注释结束
                    after_start = stripped[comment_start + 2:]
                    if '*/' in after_start:
                        # 在同一行结束
                        end_idx = after_start.find('*/') + 2
                        stripped = after_start[end_idx:].strip()
                        # 继续处理这一行的剩余部分
                    else:
                        # 多行注释跨行
                        in_multiline_comment = True
                        break
            else:
                # 没有注释，整行都是代码
                code_lines += 1
                break
    
    return code_lines

def scan_directory(directory):
    """
    扫描目录下的所有源代码文件，统计 NO_CC 块的行数
    返回: {文件路径: (行数, 块数)}
    """
    results = {}
    
    for root, dirs, files in os.walk(directory):
        # 跳过一些常见的非源代码目录
        dirs[:] = [d for d in dirs if d not in {'.git', 'build', 'cmake-build', '__pycache__', 'node_modules'}]
        
        for file in files:
            file_path = Path(root) / file
            ext = file_path.suffix.lower()
            
            if ext in SOURCE_EXTENSIONS:
                lines, blocks = count_no_cc_lines_in_file(file_path)
                if lines > 0 or blocks > 0:
                    results[str(file_path)] = (lines, blocks)
    
    return results

def scan_cpp_files_for_code_lines(directory):
    """
    扫描目录下的所有 C++/HPP 文件，统计代码行数（排除空行和注释）
    只统计真正有用的文件（排除测试、示例等）
    返回: {文件路径: 代码行数}
    """
    results = {}
    
    for root, dirs, files in os.walk(directory):
        # 跳过一些常见的非源代码目录
        dirs[:] = [d for d in dirs if d not in {'.git', 'build', 'cmake-build', '__pycache__', 'node_modules'}]
        
        for file in files:
            file_path = Path(root) / file
            ext = file_path.suffix.lower()
            
            if ext in CPP_EXTENSIONS:
                # 只统计有用的文件
                if is_useful_file(file_path):
                    code_lines = count_code_lines_in_file(file_path)
                    if code_lines > 0:
                        results[str(file_path)] = code_lines
    
    return results

def format_kloc(lines):
    """将行数格式化为 K LoC 格式，如果行数小于 1000 则显示绝对行数"""
    if lines == 0:
        return "0"
    if lines < 1000:
        return str(lines)
    return f"{lines / 1000:.2f} K LoC"

def calc_percentage(no_cc_lines, total_code_lines):
    """计算百分比"""
    if total_code_lines == 0:
        return "0.00%"
    return f"{no_cc_lines / total_code_lines * 100:.2f}%"

def main():
    # 获取脚本所在目录（仓库根目录）
    repo_root = Path(__file__).parent
    ds_dir = repo_root / 'ds'
    
    if not ds_dir.exists():
        print(f"Error: {ds_dir} does not exist!")
        return
    
    # 统计每个子目录的结果
    dir_stats = defaultdict(lambda: {'lines': 0, 'files': 0, 'blocks': 0, 'code_lines': 0, 'code_files': 0})
    
    # 遍历 ds/ 下的每个子目录
    for subdir in sorted(ds_dir.iterdir()):
        if not subdir.is_dir():
            continue
        
        print(f"Scanning {subdir.name}...")
        # 统计 NO_CC 块
        results = scan_directory(subdir)
        for file_path, (lines, blocks) in results.items():
            dir_stats[subdir.name]['lines'] += lines
            dir_stats[subdir.name]['blocks'] += blocks
            dir_stats[subdir.name]['files'] += 1
        
        # 统计代码总行数
        cpp_results = scan_cpp_files_for_code_lines(subdir)
        for file_path, code_lines in cpp_results.items():
            dir_stats[subdir.name]['code_lines'] += code_lines
            dir_stats[subdir.name]['code_files'] += 1
    
    # 输出统计结果
    print("\n" + "=" * 110)
    print("统计结果：每个 ds/ 子目录下 #ifdef NO_CC 或 #if defined(NO_CC) 块的行数")
    print("=" * 110)
    print(f"{'目录名':<30} {'NO_CC文件数':<12} {'NO_CC块数':<12} {'NO_CC行数':<12} {'代码文件数':<12} {'代码总行数':<15} {'更新百分比':<12}")
    print("-" * 110)
    
    total_lines = 0
    total_files = 0
    total_blocks = 0
    total_code_lines = 0
    total_code_files = 0
    
    def format_kloc(lines):
        """将行数格式化为 K LoC 格式，如果行数小于 1000 则显示绝对行数"""
        if lines == 0:
            return "0"
        if lines < 1000:
            return str(lines)
        return f"{lines / 1000:.2f} K LoC"
    
    def calc_percentage(no_cc_lines, total_code_lines):
        """计算百分比"""
        if total_code_lines == 0:
            return "0.00%"
        return f"{no_cc_lines / total_code_lines * 100:.2f}%"
    
    for dir_name in sorted(dir_stats.keys()):
        stats = dir_stats[dir_name]
        code_lines_str = format_kloc(stats['code_lines'])
        percentage = calc_percentage(stats['lines'], stats['code_lines'])
        print(f"{dir_name:<30} {stats['files']:<12} {stats['blocks']:<12} {stats['lines']:<12} "
              f"{stats['code_files']:<12} {code_lines_str:<15} {percentage:<12}")
        total_lines += stats['lines']
        total_files += stats['files']
        total_blocks += stats['blocks']
        total_code_lines += stats['code_lines']
        total_code_files += stats['code_files']
    
    print("-" * 110)
    total_code_lines_str = format_kloc(total_code_lines)
    total_percentage = calc_percentage(total_lines, total_code_lines)
    print(f"{'总计':<30} {total_files:<12} {total_blocks:<12} {total_lines:<12} "
          f"{total_code_files:<12} {total_code_lines_str:<15} {total_percentage:<12}")
    print("=" * 110)
    
    # 保存详细结果到文件
    output_file = repo_root / 'no_cc_lines_report.txt'
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("统计结果：每个 ds/ 子目录下 #ifdef NO_CC 或 #if defined(NO_CC) 块的行数\n")
        f.write("=" * 110 + "\n")
        f.write(f"{'目录名':<30} {'NO_CC文件数':<12} {'NO_CC块数':<12} {'NO_CC行数':<12} "
                f"{'代码文件数':<12} {'代码总行数':<15} {'更新百分比':<12}\n")
        f.write("-" * 110 + "\n")
        
        for dir_name in sorted(dir_stats.keys()):
            stats = dir_stats[dir_name]
            code_lines_str = format_kloc(stats['code_lines'])
            percentage = calc_percentage(stats['lines'], stats['code_lines'])
            f.write(f"{dir_name:<30} {stats['files']:<12} {stats['blocks']:<12} {stats['lines']:<12} "
                    f"{stats['code_files']:<12} {code_lines_str:<15} {percentage:<12}\n")
        
        f.write("-" * 110 + "\n")
        total_code_lines_str = format_kloc(total_code_lines)
        total_percentage = calc_percentage(total_lines, total_code_lines)
        f.write(f"{'总计':<30} {total_files:<12} {total_blocks:<12} {total_lines:<12} "
                f"{total_code_files:<12} {total_code_lines_str:<15} {total_percentage:<12}\n")
        f.write("=" * 110 + "\n")
    
    print(f"\n详细报告已保存到: {output_file}")

if __name__ == '__main__':
    main()

