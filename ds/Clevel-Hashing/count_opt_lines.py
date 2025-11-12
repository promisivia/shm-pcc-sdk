#!/usr/bin/env python3
"""
统计文件中所有OPT开头的宏定义块的行数
统计OPT_*块中的非注释行数（如果有#else，只统计到#else之前）
"""

import re
import sys
from pathlib import Path
from collections import defaultdict


def is_comment_line(line):
    """判断一行是否为注释行"""
    stripped = line.strip()
    # 空行不算注释
    if not stripped:
        return False
    # 单行注释：// 开头的行
    if stripped.startswith('//'):
        return True
    # 块注释：/* ... */ 在同一行
    if stripped.startswith('/*') and stripped.endswith('*/'):
        return True
    # 块注释开始：/* 开头
    if stripped.startswith('/*'):
        return True
    # 块注释结束：*/ 结尾
    if stripped.endswith('*/'):
        return True
    return False


def is_in_block_comment(lines, line_idx):
    """判断当前行是否在块注释中"""
    in_comment = False
    for i in range(line_idx + 1):
        line = lines[i].strip()
        if '/*' in line:
            # 检查是否是同一行的结束注释
            if '*/' in line:
                continue
            in_comment = True
        if '*/' in line and in_comment:
            in_comment = False
    return in_comment


def count_lines_in_block(lines, start_idx, end_idx):
    """统计块中的非注释行数（不包括#ifdef和#endif行）"""
    count = 0
    in_block_comment = False
    
    for i in range(start_idx + 1, end_idx):
        line = lines[i]
        stripped = line.strip()
        
        # 检查块注释的开始和结束
        if '/*' in stripped:
            if '*/' not in stripped:
                in_block_comment = True
        if '*/' in stripped:
            in_block_comment = False
            continue
        
        # 如果在块注释中，跳过
        if in_block_comment:
            continue
        
        # 跳过空行
        if not stripped:
            continue
        
        # 跳过单行注释
        if stripped.startswith('//'):
            continue
        
        # 跳过#ifdef、#else和#endif行
        if stripped.startswith('#ifdef') or stripped.startswith('#endif') or stripped.startswith('#else'):
            continue
        
        # 有效代码行
        count += 1
    
    return count


def find_else_or_endif(lines, ifdef_idx):
    """找到与#ifdef/#if匹配的#else或#endif，返回(#else_idx, #endif_idx)"""
    depth = 1
    i = ifdef_idx + 1
    else_idx = None
    endif_idx = None
    
    while i < len(lines) and depth > 0:
        stripped = lines[i].strip()
        # 检查是否是注释掉的#ifdef、#if、#else或#endif
        if stripped.startswith('//') or stripped.startswith('/*'):
            i += 1
            continue
        
        if stripped.startswith('#ifdef') or stripped.startswith('#if '):
            depth += 1
        elif stripped.startswith('#else') and depth == 1:
            # 只记录最外层的#else
            if else_idx is None:
                else_idx = i
        elif stripped.startswith('#endif'):
            depth -= 1
            if depth == 0:
                endif_idx = i
        i += 1
    
    return (else_idx, endif_idx)


def extract_macro_name(line):
    """从#ifdef或#if行中提取宏名称"""
    stripped = line.strip()
    
    # 处理 #ifdef MACRO_NAME
    if stripped.startswith('#ifdef '):
        macro = stripped[7:].strip()
        # 移除可能的注释
        if '//' in macro:
            macro = macro[:macro.index('//')].strip()
        if '/*' in macro:
            macro = macro[:macro.index('/*')].strip()
        return macro
    
    # 处理 #if defined(MACRO_NAME) 或 #if defined(...) && defined(MACRO_NAME)
    if stripped.startswith('#if '):
        # 查找 defined(OPT_...)
        match = re.search(r'defined\s*\(\s*(OPT_\w+)\s*\)', stripped)
        if match:
            return match.group(1)
    
    return None


def count_all_opt_blocks(file_path):
    """统计文件中所有OPT开头的宏定义块"""
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # 使用字典存储每个宏的块
    macro_blocks = defaultdict(list)
    
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        # 查找 #ifdef OPT_* 或 #if defined(OPT_*)
        if stripped.startswith('#ifdef ') or stripped.startswith('#if '):
            # 确保不是注释行
            if not is_comment_line(line) and not is_in_block_comment(lines, i):
                macro_name = extract_macro_name(line)
                
                # 只处理OPT开头的宏
                if macro_name and macro_name.startswith('OPT_'):
                    else_idx, endif_idx = find_else_or_endif(lines, i)
                    if endif_idx is not None:
                        # 如果有#else，只统计到#else之前；否则统计到#endif
                        end_idx = else_idx if else_idx is not None else endif_idx
                        line_count = count_lines_in_block(lines, i, end_idx)
                        macro_blocks[macro_name].append({
                            'start': i + 1,  # 1-based line number
                            'end': endif_idx + 1,
                            'else_line': else_idx + 1 if else_idx is not None else None,
                            'lines': line_count
                        })
                        i = endif_idx
        i += 1
    
    return macro_blocks


def main():
    if len(sys.argv) > 1:
        file_path = Path(sys.argv[1])
    else:
        # 默认使用当前目录下的 clevel_hash.hpp
        script_dir = Path(__file__).parent
        file_path = script_dir / 'include' / 'libpmemobj++' / 'experimental' / 'clevel_hash.hpp'
    
    if not file_path.exists():
        print(f"Error: {file_path} not found!")
        sys.exit(1)
    
    print(f"Analyzing: {file_path}\n")
    
    # 统计所有OPT开头的宏
    all_opt_blocks = count_all_opt_blocks(file_path)
    
    if not all_opt_blocks:
        print("No OPT_* macros found in the file.")
        return
    
    # 按宏名称排序
    sorted_macros = sorted(all_opt_blocks.keys())
    
    total_all_lines = 0
    for macro_name in sorted_macros:
        blocks = all_opt_blocks[macro_name]
        total_lines = sum(block['lines'] for block in blocks)
        total_all_lines += total_lines
        
        print("=" * 60)
        print(f"{macro_name} blocks:")
        print("=" * 60)
        for i, block in enumerate(blocks, 1):
            if block['else_line']:
                print(f"  Block {i}: lines {block['start']}-{block['else_line']} (before #else): {block['lines']} lines")
            else:
                print(f"  Block {i}: lines {block['start']}-{block['end']}: {block['lines']} lines")
        print(f"\n  Total blocks: {len(blocks)}")
        print(f"  Total lines: {total_lines}")
        print()
    
    print("=" * 60)
    print("Summary:")
    print("=" * 60)
    for macro_name in sorted_macros:
        blocks = all_opt_blocks[macro_name]
        total_lines = sum(block['lines'] for block in blocks)
        print(f"{macro_name:30s}: {total_lines:4d} lines in {len(blocks)} blocks")
    print(f"\n{'Total OPT_* lines':30s}: {total_all_lines:4d} lines")


if __name__ == '__main__':
    main()

