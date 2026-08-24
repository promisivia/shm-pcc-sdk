# 开源就绪状态

CXL-SDK 使用仓库内的 open-source readiness harness 持续检查开源质量，
避免维护一份很快过期的人工审计报告。

## 当前自动化基线

```bash
./tools/opensource-harness/run.sh
./tools/opensource-harness/run.sh --full
```

快速模式检查社区文件、仓库布局、生成物、Markdown 链接、品牌占位符、
高置信度密钥模式、维护脚本语法与 Git 空白错误。完整模式额外执行干净的
`shm-lib` CMake 配置/构建和严格 Sphinx 构建。

CI 在 `master`、`opensource` 和面向 `master` 的 pull request 上运行完整模式。

## 已纳入基线

- 根目录许可证、贡献指南、行为准则、安全策略、支持说明和第三方声明；
- GitHub issue / pull request 模板；
- 不允许提交对象文件、静态库、可执行文件、Python 缓存和打包镜像；
- README 与主文档中的本地链接、品牌名称和占位内容；
- 可从干净 checkout 配置并构建核心库；
- Sphinx 文档在 CI 中以 warning-as-error 模式构建。

## 仍需人工审查

自动化不能替代以下工作：

- 第三方许可证兼容性和来源确认；
- Git 历史中的敏感信息与大型对象清理；
- CXL/UB 硬件、NUMA 拓扑和多机环境的正确性验证；
- 性能结果的方法学、重复性与统计显著性；
- 公开 API、共享内存布局和故障恢复协议的兼容性评审。

每次准备 release 时，应保存 harness 的 JSON 报告，并在 release notes 中记录
无法在 CI 覆盖的硬件验证。
