# BwTree 树结构可视化功能

## 概述

我为BwTree添加了一套完整的树结构遍历和可视化功能，可以帮助开发者调试和分析BwTree的内部结构。

## 新增功能

### 1. PrintTreeStructure() - 主要树结构可视化函数

```cpp
void PrintTreeStructure(std::ostream& os = std::cout, bool show_deltas = true, int max_depth = -1) const;
```

**功能**: 从根节点开始，递归遍历整个BwTree，输出可视化的树结构。

**参数**:
- `os`: 输出流，默认为stdout
- `show_deltas`: 是否显示delta链信息，默认为true
- `max_depth`: 最大显示深度，默认为-1（无限制）

**输出示例**:
```
=== BwTree Structure Visualization ===
Root Node ID: 1
Next Node ID: 15

Node 1 (Inner)
  Depth: 0
  Item Count: 2
  Low Key: 3
  High Key: 30
  Children:
  ├─ Node 2 (Leaf)
  │   Depth: 1
  │   Item Count: 3
  │   Low Key: 3
  │   High Key: 7
  ├─ Node 3 (Leaf)
  │   Depth: 1
  │   Item Count: 4
  │   Low Key: 10
  │   High Key: 18
  └─ Node 4 (Leaf)
      Depth: 1
      Item Count: 3
      Low Key: 20
      High Key: 30
```

### 2. PrintTreeStatistics() - 树统计信息

```cpp
void PrintTreeStatistics(std::ostream& os = std::cout) const;
```

**功能**: 统计整个树的节点数量和类型分布。

**输出示例**:
```
=== BwTree Statistics ===
Total Nodes: 4
Node Type Distribution:
  Inner: 1
  Leaf: 3
Next Node ID: 15
=== End of Statistics ===
```

### 3. 辅助函数

- `PrintNodeRecursive()`: 递归打印单个节点及其子树
- `PrintNodeDetails()`: 打印节点的详细信息
- `PrintInnerNodeChildren()`: 打印内部节点的子节点
- `PrintDeltaChain()`: 打印delta链信息
- `GetNodeTypeString()`: 将节点类型转换为可读字符串
- `CollectTreeStatistics()`: 收集树的统计信息

## 使用方法

### 基本用法

```cpp
#include "bwtree.h"

// 创建BwTree实例
BwTree<int, std::string> tree;

// 插入一些数据
tree.Insert(10, "value_10");
tree.Insert(20, "value_20");
tree.Insert(5, "value_5");

// 打印完整树结构
tree.PrintTreeStructure();

// 打印统计信息
tree.PrintTreeStatistics();
```

### 高级用法

```cpp
// 限制显示深度为2层
tree.PrintTreeStructure(std::cout, true, 2);

// 不显示delta链信息
tree.PrintTreeStructure(std::cout, false);

// 输出到文件
std::ofstream file("tree_structure.txt");
tree.PrintTreeStructure(file);
file.close();
```

## 输出格式说明

### 树结构格式

- `├─`: 表示有后续兄弟节点
- `└─`: 表示最后一个节点
- 缩进表示层级关系
- 每个节点显示ID、类型、深度、项目数量等信息

### 节点信息

- **Node ID**: 节点的唯一标识符
- **Type**: 节点类型（Inner、Leaf、InnerInsert等）
- **Depth**: 节点在树中的深度
- **Item Count**: 节点包含的项目数量
- **Key Range**: 节点的键值范围
- **Delta Chain**: delta链信息（如果启用）

## 调试建议

1. **开发阶段**: 使用`PrintTreeStructure()`来验证树的结构是否正确
2. **性能分析**: 使用`PrintTreeStatistics()`来了解树的规模和分布
3. **问题排查**: 结合深度限制和delta链信息来定位问题
4. **日志记录**: 将输出重定向到日志文件进行长期监控

## 注意事项

1. **性能影响**: 这些函数会遍历整个树，在大型树上可能较慢
2. **并发安全**: 在并发环境中使用时要小心，树结构可能在遍历过程中发生变化
3. **内存使用**: 统计功能会记录已访问的节点，大型树可能消耗较多内存
4. **输出格式**: 输出格式为纯文本，适合日志记录和调试，不适合图形化显示

## 扩展功能

如果需要更高级的可视化功能，可以考虑：

1. **图形化输出**: 生成DOT格式文件，使用Graphviz渲染
2. **JSON输出**: 生成结构化数据，便于程序化处理
3. **实时监控**: 集成到监控系统中，定期输出树结构
4. **性能指标**: 添加节点访问频率、分裂次数等统计信息

## 示例程序

参考 `test_tree_visualization.cpp` 文件，其中包含了完整的使用示例。
