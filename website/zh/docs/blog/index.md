# 技术博客

<div class="blog-hero">
  <div class="blog-kicker">CXL-SDK / Technical Blog</div>
  <h1>从硬件一致性到系统软件设计。</h1>
  <p>用面向系统开发者的科普与技术长文解释 CXL、UB、共享内存和并发系统中的关键概念，也记录 CXL-SDK 各模块背后的设计取舍。</p>
</div>

## 文章

<div class="blog-grid">
  <a class="blog-card" href="hardware-coherence.html">
    <span class="card-number">01 / COHERENCE</span>
    <h3>硬件一致性模型</h3>
    <p>从缓存一致性与内存一致性出发，建立理解 CXL、UB 等系统的共同基础。</p>
    <span class="card-status">待撰写 →</span>
  </a>
  <a class="blog-card" href="cxl-and-ub.html">
    <span class="card-number">02 / INTERCONNECT</span>
    <h3>CXL 与 UB</h3>
    <p>梳理两类互连技术解决的问题、系统形态，以及软件真正能依赖的语义。</p>
    <span class="card-status">待撰写 →</span>
  </a>
  <a class="blog-card" href="architecture-map.html">
    <span class="card-number">03 / CXL-SDK</span>
    <h3>CXL-SDK 架构地图</h3>
    <p>从应用、数据结构、运行时到内存后端，理解仓库的整体设计。</p>
    <span class="card-status">已有骨架 →</span>
  </a>
</div>

## 后续专题

共享内存运行时、内存分配、并发数据结构、通信、并发控制，以及应用评测会作为独立专题逐步展开。每篇文章重点解释问题、原理和设计取舍；具体的接口与开发步骤仍放在开发者文档和 API Reference 中。

```{toctree}
:maxdepth: 1

hardware-coherence
cxl-and-ub
architecture-map
```
