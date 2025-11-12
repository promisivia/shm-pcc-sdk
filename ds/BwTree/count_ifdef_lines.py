#!/usr/bin/env python3
"""
统计BwTree中特定#ifdef块的行数
统计OPT_ROOT_READ和OPT_IN_USE_FLAG块中的非注释行数
"""

import re
import sys
from pathlib import Path


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
        
        # 跳过#ifdef、#else和#endif行（虽然理论上不应该出现在中间）
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


def is_single_no_cc(line):
    """检查是否是单独的NO_CC定义（不包括组合条件）"""
    stripped = line.strip()
    
    # 处理 #ifdef NO_CC
    if stripped == '#ifdef NO_CC' or stripped.startswith('#ifdef NO_CC '):
        return True
    
    # 处理 #if defined(NO_CC) - 必须是单独的，不能有 && 或 ||
    if stripped.startswith('#if '):
        # 检查是否是 #if defined(NO_CC) 且没有 && 或 ||
        pattern = r'^#if\s+defined\s*\(\s*NO_CC\s*\)\s*$'
        if re.match(pattern, stripped):
            return True
    
    return False


def count_ifdef_blocks(file_path, macro_name):
    """统计特定宏的所有块的行数"""
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    blocks = []
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        # 对于NO_CC，只统计单独定义的
        if macro_name == 'NO_CC':
            if is_single_no_cc(line):
                # 确保不是注释行
                if not is_comment_line(line) and not is_in_block_comment(lines, i):
                    else_idx, endif_idx = find_else_or_endif(lines, i)
                    if endif_idx is not None:
                        end_idx = else_idx if else_idx is not None else endif_idx
                        line_count = count_lines_in_block(lines, i, end_idx)
                        blocks.append({
                            'start': i + 1,
                            'end': endif_idx + 1,
                            'else_line': else_idx + 1 if else_idx is not None else None,
                            'lines': line_count
                        })
                        i = endif_idx
        else:
            # 查找 #ifdef MACRO_NAME 或 #if defined(MACRO_NAME) 或 #if defined(...) && defined(MACRO_NAME)
            pattern1 = f'#ifdef {macro_name}'
            pattern2 = f'defined({macro_name})'
            
            is_match = False
            if stripped == pattern1 or stripped.startswith(pattern1 + ' '):
                is_match = True
            elif pattern2 in stripped and (stripped.startswith('#if ') or stripped.startswith('#ifdef ')):
                is_match = True
            
            if is_match:
                # 确保不是注释行
                if not is_comment_line(line) and not is_in_block_comment(lines, i):
                    else_idx, endif_idx = find_else_or_endif(lines, i)
                    if endif_idx is not None:
                        end_idx = else_idx if else_idx is not None else endif_idx
                        line_count = count_lines_in_block(lines, i, end_idx)
                        blocks.append({
                            'start': i + 1,
                            'end': endif_idx + 1,
                            'else_line': else_idx + 1 if else_idx is not None else None,
                            'lines': line_count
                        })
                        i = endif_idx
        i += 1
    
    return blocks


def main():
    script_dir = Path(__file__).parent
    bwtree_file = script_dir / 'src' / 'bwtree.h'
    
    if not bwtree_file.exists():
        print(f"Error: {bwtree_file} not found!")
        sys.exit(1)
    
    print(f"Analyzing: {bwtree_file}\n")
    
    # 统计 OPT_ROOT_READ
    print("=" * 60)
    print("OPT_ROOT_READ blocks:")
    print("=" * 60)
    opt_root_blocks = count_ifdef_blocks(bwtree_file, 'OPT_ROOT_READ')
    total_root_lines = 0
    for i, block in enumerate(opt_root_blocks, 1):
        if block['else_line']:
            print(f"  Block {i}: lines {block['start']}-{block['else_line']} (before #else): {block['lines']} lines")
        else:
            print(f"  Block {i}: lines {block['start']}-{block['end']}: {block['lines']} lines")
        total_root_lines += block['lines']
    print(f"\n  Total blocks: {len(opt_root_blocks)}")
    print(f"  Total lines: {total_root_lines}")
    
    # 统计 OPT_IN_USE_FLAG
    print("\n" + "=" * 60)
    print("OPT_IN_USE_FLAG blocks:")
    print("=" * 60)
    opt_use_blocks = count_ifdef_blocks(bwtree_file, 'OPT_IN_USE_FLAG')
    total_use_lines = 0
    for i, block in enumerate(opt_use_blocks, 1):
        if block['else_line']:
            print(f"  Block {i}: lines {block['start']}-{block['else_line']} (before #else): {block['lines']} lines")
        else:
            print(f"  Block {i}: lines {block['start']}-{block['end']}: {block['lines']} lines")
        total_use_lines += block['lines']
    print(f"\n  Total blocks: {len(opt_use_blocks)}")
    print(f"  Total lines: {total_use_lines}")
    
    # 统计 OPT_GC
    print("\n" + "=" * 60)
    print("OPT_GC blocks:")
    print("=" * 60)
    opt_gc_blocks = count_ifdef_blocks(bwtree_file, 'OPT_GC')
    total_gc_lines = 0
    for i, block in enumerate(opt_gc_blocks, 1):
        if block['else_line']:
            print(f"  Block {i}: lines {block['start']}-{block['else_line']} (before #else): {block['lines']} lines")
        else:
            print(f"  Block {i}: lines {block['start']}-{block['end']}: {block['lines']} lines")
        total_gc_lines += block['lines']
    print(f"\n  Total blocks: {len(opt_gc_blocks)}")
    print(f"  Total lines: {total_gc_lines}")
    
    # 统计 NO_CC (只统计单独定义的)
    print("\n" + "=" * 60)
    print("NO_CC blocks (single definition only):")
    print("=" * 60)
    no_cc_blocks = count_ifdef_blocks(bwtree_file, 'NO_CC')
    total_no_cc_lines = 0
    for i, block in enumerate(no_cc_blocks, 1):
        if block['else_line']:
            print(f"  Block {i}: lines {block['start']}-{block['else_line']} (before #else): {block['lines']} lines")
        else:
            print(f"  Block {i}: lines {block['start']}-{block['end']}: {block['lines']} lines")
        total_no_cc_lines += block['lines']
    print(f"\n  Total blocks: {len(no_cc_blocks)}")
    print(f"  Total lines: {total_no_cc_lines}")
    
    print("\n" + "=" * 60)
    print("Summary:")
    print("=" * 60)
    print(f"OPT_ROOT_READ:   {total_root_lines} lines in {len(opt_root_blocks)} blocks")
    print(f"OPT_IN_USE_FLAG: {total_use_lines} lines in {len(opt_use_blocks)} blocks")
    print(f"OPT_GC:          {total_gc_lines} lines in {len(opt_gc_blocks)} blocks")
    print(f"NO_CC:           {total_no_cc_lines} lines in {len(no_cc_blocks)} blocks")


if __name__ == '__main__':
    main()

