# HOT 数据结构文档

## 概述

HOT (Height Optimized Trie) 是一个高度优化的 Trie 索引结构，使用 ROWEX (Read-Optimized Write EXclusion) 并发控制协议。本文档详细说明 HOT 的核心数据结构组织。

---

## 核心数据结构

### 1. `mPointer` - 编码指针 (HOTRowexChildPointer)

`mPointer` 是一个编码的原子指针，可以表示内部节点或叶子值。

#### 编码格式

```
mPointer 的位编码：
┌─────────────────────────────────────────┐
│  bits 4+  │ bits 1-3 │ bit 0 │
│  指针/值  │ 节点类型 │ 叶子标记 │
└─────────────────────────────────────────┘
```

- **bit 0 (最低位)**：
  - `0` = 内部节点 (Inner Node)
  - `1` = 叶子值 (Leaf Value/TID)

- **bits 1-3**：节点类型 (NodeType)
  - 8 种不同的节点类型，取决于 `DiscriminativeBitsRepresentation` 和 `PartialKeyType`
  - 例如：`SINGLE_MASK_8_BIT_PARTIAL_KEYS`, `MULTI_MASK_8_BYTES_AND_16_BIT_PARTIAL_KEYS` 等

- **bits 4+**：
  - 如果是节点：存储节点指针地址
  - 如果是叶子：存储 TID (Tuple ID，最多 63 位)

#### 代码示例

```cpp
// 创建指向节点的 ChildPointer
HOTRowexChildPointer(hot::commons::NodeType nodeType, HOTRowexNodeBase const *node)
    : mPointer((reinterpret_cast<intptr_t>(node) | static_cast<intptr_t>(nodeType)) << 1)

// 创建指向叶子值的 ChildPointer
HOTRowexChildPointer(intptr_t leafValue)
    : mPointer((leafValue << 1) | 1)  // 最低位设为 1 表示叶子
```

#### 解码方法

```cpp
// 获取节点类型
hot::commons::NodeType getNodeType() const {
    const unsigned int nodeAlgorithmCode = 
        static_cast<unsigned int>(mPointer.load(read_memory_order) & NODE_ALGORITH_TYPE_HELPER_EXTRACTION_MASK);
    return static_cast<hot::commons::NodeType>(nodeAlgorithmCode >> 1u);
}

// 获取节点指针
HOTRowexNodeBase* getNode() const {
    intptr_t const nodePointerValue = 
        (mPointer.load(read_memory_order) >> 1) & POINTER_EXTRACTION_MASK;
    return reinterpret_cast<HOTRowexNodeBase *>(nodePointerValue);
}

// 获取叶子值 (TID)
intptr_t getTid() const {
    return mPointer.load(read_memory_order) >> 1;
}
```

---

### 2. Inner Node - HOTRowexNode

HOT 的内部节点采用继承结构，基类包含通用字段，派生类包含特定于节点类型的字段。

#### 继承关系

```
HOTRowexNodeBase (基类)
    ↓
HOTRowexNode<DiscriminativeBitsRepresentation, PartialKeyType> (派生类)
```

#### HOTRowexNodeBase (基类)

```cpp
struct alignas(SIMD_COB_TRIE_NODE_ALIGNMENT) HOTRowexNodeBase {
    // 指向子指针数组的第一个元素
    HOTRowexChildPointer* mFirstChildPointer;
    
    // 已使用条目的位掩码
    // bit 0 (最低位) 对应索引 0，bit 31 (最高位) 对应索引 31
    uint32_t mUsedEntriesMask;
    
    // 节点高度（叶子节点高度为 1，其父节点高度为 2，以此类推）
    uint16_t const mHeight;
    
    // 自旋锁（用于写操作互斥）
    SpinLock mLock;
    
    // 废弃标记（原子布尔值，用于标记节点是否已被替换）
    std::atomic<bool> mIsObsolete;
};
```

**重要说明**：`mLock` 是 `SpinLock` 类型，用于写操作的互斥访问。在 `NO_CC` 模式下，`SpinLock` 使用 `nt<uint32_t>`（非临时存储）实现，确保锁状态直接写入持久内存。

#### HOTRowexNode (派生类)

```cpp
template<typename DiscriminativeBitsRepresentation, typename PartialKeyType>
struct HOTRowexNode : HOTRowexNodeBase {
    // 判别位表示（SingleMask 或 MultiMask）
    DiscriminativeBitsRepresentation mDiscriminativeBitsRepresentation;
    
    // 稀疏部分键数组
    hot::commons::SparsePartialKeys<PartialKeyType> mPartialKeys;
};
```

#### 锁操作 (tryLock/unlock)

```cpp
inline bool HOTRowexNodeBase::tryLock() {
    bool aquiredLock = false;
    if(!isObsolete()) {
        mLock.lock();  // 获取 SpinLock
        if(isObsolete()) {
            mLock.unlock();
        } else {
            aquiredLock = true;
        }
    }
#ifdef NO_CC
    if (aquiredLock) {
        // 在 NO_CC 模式下，确保锁状态持久化
        clflush((char *)this, sizeof(HOTRowexNodeBase));
    }
#endif
    return aquiredLock;
}

inline void HOTRowexNodeBase::unlock() {
#ifdef NO_CC
    // 在 NO_CC 模式下，确保解锁状态持久化
    clflush((char *)this, sizeof(HOTRowexNodeBase));
#endif
    mLock.unlock();
}
```

**注意**：在 `NO_CC` 模式下，`tryLock()` 和 `unlock()` 都会调用 `clflush` 来确保锁状态的变化被刷新到持久内存，这对于持久内存系统（如 CXL）的正确性至关重要。

---

### 3. PartialKey - 稀疏部分键

PartialKey 存储在 `SparsePartialKeys<PartialKeyType>` 结构中，用于高效搜索。

#### 结构定义

```cpp
template<typename PartialKeyType>
struct alignas(8) SparsePartialKeys {
    PartialKeyType mEntries[1];  // 可变长度数组
};
```

#### 特点

1. **稀疏存储**：只存储判别位（discriminative bits），而不是完整的键
2. **SIMD 优化**：使用 AVX2/AVX512 指令进行并行搜索
3. **类型支持**：支持 `uint8_t`、`uint16_t`、`uint32_t` 三种类型

#### 稀疏部分键示例

假设有一个二进制 Patricia Trie，包含以下键：
- `v1`: `0110100101` (判别位: {3,6,8})
- `v2`: `0110100110` (判别位: {3,6,8})
- `v3`: `0110101010` (判别位: {3,6,9})

稀疏部分键只存储这些判别位的值，而不是完整的键。

#### SIMD 搜索

```cpp
template<>
inline uint32_t SparsePartialKeys<uint8_t>::search(uint8_t const uncompressedSearchMask) const {
    __m256i searchRegister = _mm256_set1_epi8(uncompressedSearchMask);
    __m256i haystack = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries));
    
#ifdef USE_AVX512
    uint32_t const resultMask = _mm256_cmpeq_epi8_mask(
        _mm256_and_si256(haystack, searchRegister), haystack);
#else
    __m256i searchResult = _mm256_cmpeq_epi8(
        _mm256_and_si256(haystack, searchRegister), haystack);
    uint32_t const resultMask = static_cast<uint32_t>(_mm256_movemask_epi8(searchResult));
#endif
    return resultMask;
}
```

---

### 4. Leaf Node - 不存在独立结构

**重要**：HOT **没有独立的叶子节点结构**。叶子值直接编码在 `HOTRowexChildPointer` 的 `mPointer` 中。

#### 叶子值存储

- **叶子值 = TID (Tuple ID)**：最多 63 位（因为最低位用于标记）
- **存储位置**：`mPointer` 的高位中
- **识别方法**：检查 `mPointer` 的最低位是否为 1

```cpp
inline bool HOTRowexChildPointer::isLeaf() const {
    return mPointer.load(read_memory_order) & 1;  // 最低位为 1 表示叶子
}

inline intptr_t HOTRowexChildPointer::getTid() const {
    return mPointer.load(read_memory_order) >> 1;  // 右移 1 位获取 TID
}
```

这种设计的优势：
- **节省内存**：不需要为每个叶子值分配独立的节点结构
- **统一接口**：节点和叶子值使用相同的 `HOTRowexChildPointer` 接口
- **高效访问**：叶子值的访问只需要一次内存读取

---

## 内存布局

### 节点内存布局（从低地址到高地址）

```
┌─────────────────────────────────────────────────────────┐
│  HOTRowexNodeBase (基类部分)                            │
│  ├─ mFirstChildPointer (8 bytes)                        │ ← 指向子指针数组
│  ├─ mUsedEntriesMask (4 bytes)                          │ ← 已使用条目的位掩码
│  ├─ mHeight (2 bytes)                                    │ ← 节点高度
│  ├─ mLock (SpinLock, 大小取决于实现)                    │ ← 自旋锁
│  └─ mIsObsolete (1 byte, atomic<bool>)                  │ ← 废弃标记
├─────────────────────────────────────────────────────────┤
│  HOTRowexNode (派生类部分)                               │
│  ├─ mDiscriminativeBitsRepresentation                   │ ← 判别位表示
│  │   (SingleMaskPartialKeyMapping 或                    │
│  │    MultiMaskPartialKeyMapping)                       │
│  └─ mPartialKeys                                         │ ← 稀疏部分键数组
│      ├─ mEntries[0] (PartialKeyType)                    │
│      ├─ mEntries[1] (PartialKeyType)                    │
│      ├─ ...                                              │
│      └─ mEntries[n-1] (PartialKeyType)                  │
├─────────────────────────────────────────────────────────┤
│  Child Pointers Array (子指针数组)                      │
│  ├─ HOTRowexChildPointer[0] (8 bytes)                   │ ← 子指针 0
│  ├─ HOTRowexChildPointer[1] (8 bytes)                   │ ← 子指针 1
│  ├─ ...                                                  │
│  └─ HOTRowexChildPointer[n-1] (8 bytes)                │ ← 子指针 n-1
└─────────────────────────────────────────────────────────┘
```

### 内存分配计算

```cpp
template<typename DiscriminativeBitsRepresentation, typename PartialKeyType>
hot::commons::NodeAllocationInformation 
HOTRowexNode<...>::getNodeAllocationInformation(uint16_t const numberEntries) {
    // 计算各部分大小
    constexpr uint32_t entriesMasksBaseSize = 
        static_cast<uint32_t>(sizeof(hot::commons::SparsePartialKeys<PartialKeyType>));
    constexpr uint32_t baseSize = 
        static_cast<uint32_t>(sizeof(HOTRowexNode<...>)) - entriesMasksBaseSize;
    
    // 计算子指针数组大小（至少 3 个，或 numberEntries 个）
    uint32_t pointersSize = calculatePointerSize(numberEntries);
    
    // 计算指针偏移量（PartialKeys 数组之后）
    uint16_t pointerOffset = 
        hot::commons::SparsePartialKeys<PartialKeyType>::estimateSize(numberEntries) + baseSize;
    
    // 总大小（必须是 8 字节对齐）
    uint32_t rawSize = pointersSize + pointerOffset;
    assert((rawSize % 8) == 0);
    
    return hot::commons::NodeAllocationInformation(
        convertNumbeEntriesToEntriesMask(numberEntries), 
        rawSize, 
        pointerOffset
    );
}
```

---

## 数据结构组织关系图

```
                    HOTRowex (根索引)
                         │
                         │ mRoot (HOTRowexChildPointer)
                         │
                         ▼
              ┌──────────────────────────────┐
              │  HOTRowexNode                │ (Inner Node)
              │  ├─ mHeight                  │
              │  ├─ mUsedEntriesMask          │
              │  ├─ mLock (SpinLock)          │ ← 写操作互斥锁
              │  ├─ mIsObsolete               │
              │  ├─ mDiscriminativeBitsRep    │
              │  ├─ mPartialKeys[]            │ ← PartialKey 数组
              │  └─ mFirstChildPointer        │
              └──────────────────────────────┘
                         │
                         │ 指向子指针数组
                         ▼
        ┌────────────────────────────────────┐
        │  HOTRowexChildPointer[]            │
        │  [0] [1] [2] ... [n-1]            │
        └────────────────────────────────────┘
                 │    │    │      │
                 │    │    │      │
        ┌────────┘    │    │      └────────┐
        │             │    │               │
        ▼             ▼    ▼               ▼
    [Node]        [Leaf]  [Node]       [Leaf]
    (mPointer      (TID)   (mPointer     (TID)
    编码)                   编码)
```

---

## 关键数据结构总结

| 数据结构 | 位置 | 作用 | 特点 |
|---------|------|------|------|
| **mPointer** | `HOTRowexChildPointer` | 编码指针：节点类型 + 节点地址 或 叶子值 | 统一表示节点和叶子，节省内存 |
| **PartialKey** | `SparsePartialKeys.mEntries[]` | 稀疏部分键数组，用于 SIMD 搜索 | 只存储判别位，SIMD 优化 |
| **Inner Node** | `HOTRowexNode` | 内部节点，包含判别位、部分键、子指针 | 模板化设计，支持多种节点类型 |
| **Leaf Node** | 不存在 | 叶子值直接编码在 `mPointer` 中 | 无需独立结构，节省内存 |
| **SpinLock** | `HOTRowexNodeBase.mLock` | 写操作互斥锁 | 在 NO_CC 模式下使用非临时存储 |

---

## 设计特点

1. **无独立叶子节点**：叶子值直接编码在 `mPointer` 中，节省内存
2. **编码指针**：统一表示节点和叶子，简化接口
3. **稀疏部分键**：只存储判别位，优化存储和搜索性能
4. **紧凑内存布局**：连续存储，提高缓存效率
5. **SIMD 优化**：使用 AVX2/AVX512 指令并行搜索部分键
6. **模板化设计**：支持多种节点类型，根据数据特征选择最优类型
7. **持久内存支持**：在 `NO_CC` 模式下，使用 `clflush` 确保关键状态持久化

---

## 相关文件

- `HOTRowexChildPointer.hpp` - 编码指针实现
- `HOTRowexNodeBase.hpp` - 节点基类实现
- `HOTRowexNode.hpp` - 节点实现
- `SparsePartialKeys.hpp` - 稀疏部分键实现
- `SpinLock.hpp` - 自旋锁实现

---

---

## 读操作与写操作的锁机制

### 读操作（Lookup）- 完全无锁

**读操作完全不获取锁**，这是 ROWEX 协议的核心特性。

#### Lookup 实现

```cpp
template<typename ValueType, template <typename> typename KeyExtractor>
inline idx::contenthelpers::OptionalValue<ValueType> 
HOTRowex<ValueType, KeyExtractor>::lookup(KeyType const &key) const {
    MemoryGuard memoryGuard(mMemoryReclamation);  // 仅用于内存回收保护
    auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(
        idx::contenthelpers::toBigEndianByteOrder(key));
    uint8_t const* byteKey = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

    HOTRowexChildPointer current = mRoot;
    while(!current.isLeaf()) {
        current = *(current.search(byteKey));  // 无锁搜索
    }
    ValueType const & value = idx::contenthelpers::tidToValue<ValueType>(current.getTid());
    return { idx::contenthelpers::contentEquals(extractKey(value), key), value };
}
```

**关键点**：
- ✅ **无锁读取**：`lookup()` 不调用任何 `tryLock()` 或 `lock()`
- ✅ **MemoryGuard**：仅用于 Epoch-Based Memory Reclamation，不提供互斥保护
- ✅ **Lock-Free 搜索**：`node.search()` 完全无锁

### 写操作（Insert/Update）- 需要锁

写操作需要获取节点的 `SpinLock`：

```cpp
// 在 insert 操作中
if(parentEntry->tryLock()) {  // 获取锁
    // 修改节点内容
    leafEntry.updateChildPointer(...);
    parentEntry->unlock();  // 释放锁
}
```

---

## Lookup 过程中的节点访问模式

### Inner Node 访问的变量

在 `lookup` 过程中，每个内部节点会依次读取以下变量：

#### 1. `node.search()` 调用链

```cpp
// HOTRowexNode::search()
inline HOTRowexChildPointer const * search(uint8_t const * keyBytes) const {
    return this->getPointers() + this->toResultIndex(
        mPartialKeys.search(
            mDiscriminativeBitsRepresentation.extractMask(keyBytes)
        )
    );
}
```

#### 2. 读取的变量列表

| 变量 | 位置 | 用途 |
|------|------|------|
| **mDiscriminativeBitsRepresentation** | `HOTRowexNode` | 判别位表示，用于提取搜索掩码 |
| **mPartialKeys.mEntries[]** | `SparsePartialKeys` | 稀疏部分键数组，通过 SIMD 搜索匹配项 |
| **mUsedEntriesMask** | `HOTRowexNodeBase` | 已使用条目掩码，在 `toResultIndex()` 中使用 |
| **mFirstChildPointer** | `HOTRowexNodeBase` | 子指针数组的起始地址 |

#### 3. 访问顺序

```
node.search(keyBytes)
  ↓
1. mDiscriminativeBitsRepresentation.extractMask(keyBytes)
   └─ 读取判别位表示的字段（如 mOffsetInBytes, mSuccessiveExtractionMask）
  ↓
2. mPartialKeys.search(densePartialKey)
   └─ 读取 mPartialKeys.mEntries[] 数组（SIMD 并行搜索）
  ↓
3. toResultIndex(resultMask)
   └─ 读取 mUsedEntriesMask
  ↓
4. getPointers() + resultIndex
   └─ 读取 mFirstChildPointer，然后访问子指针数组
```

**总结**：每个内部节点在搜索过程中会读取 **4 个主要数据结构**，以及通过 SIMD 访问的部分键数组。

### Leaf Node 访问

**叶子节点只读取一个位置**：`mPointer`

```cpp
// 检查是否为叶子
inline bool HOTRowexChildPointer::isLeaf() const {
    return mPointer.load(read_memory_order) & 1;  // 读取 mPointer
}

// 获取叶子值 (TID)
inline intptr_t HOTRowexChildPointer::getTid() const {
    return mPointer.load(read_memory_order) >> 1;  // 读取 mPointer
}
```

**关键点**：
- ✅ **单一读取**：叶子值直接编码在 `mPointer` 中
- ✅ **原子读取**：使用 `memory_order_acquire` 确保可见性
- ✅ **无额外结构**：不需要访问任何节点结构

---

## 读操作的内存访问模式总结

### Inner Node（内部节点）

```
每次 lookup 访问一个内部节点时：
┌─────────────────────────────────────────┐
│  读取操作（无锁）                        │
├─────────────────────────────────────────┤
│  1. mDiscriminativeBitsRepresentation   │ ← 判别位表示
│  2. mPartialKeys.mEntries[]             │ ← 部分键数组（SIMD）
│  3. mUsedEntriesMask                    │ ← 已使用条目掩码
│  4. mFirstChildPointer                  │ ← 子指针数组起始地址
│  5. ChildPointer[resultIndex]           │ ← 选中的子指针
└─────────────────────────────────────────┘
```

### Leaf Node（叶子节点）

```
每次 lookup 访问一个叶子节点时：
┌─────────────────────────────────────────┐
│  读取操作（无锁）                        │
├─────────────────────────────────────────┤
│  1. mPointer (atomic)                    │ ← 唯一读取位置
│     └─ 包含 TID (叶子值)                 │
└─────────────────────────────────────────┘
```

### 完整的 Lookup 路径示例

```
lookup(key)
  ↓
mRoot (HOTRowexChildPointer)
  ↓ [读取 mPointer]
Node1.search()
  ↓ [读取: mDiscriminativeBits, mPartialKeys[], mUsedEntriesMask, mFirstChildPointer]
ChildPointer[2]
  ↓ [读取 mPointer]
Node2.search()
  ↓ [读取: mDiscriminativeBits, mPartialKeys[], mUsedEntriesMask, mFirstChildPointer]
ChildPointer[5]
  ↓ [读取 mPointer]
Leaf (TID)
  ↓ [只读取 mPointer]
返回 TID
```

---

## 参考资料

- HOT 论文："HOT: A Height Optimized Trie Index for Main-Memory Database Systems" (SIGMOD 2018)
- ROWEX 协议：Read-Optimized Write EXclusion 并发控制协议

