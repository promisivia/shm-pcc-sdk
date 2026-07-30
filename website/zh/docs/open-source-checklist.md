# 开源发布清单

## 每个 pull request

- [ ] 改动范围清晰，不包含本地构建产物或打包镜像。
- [ ] API、配置和行为变化已同步更新文档。
- [ ] 新增第三方代码已记录许可证与来源。
- [ ] 已运行 `./tools/opensource-harness/run.sh`。
- [ ] 硬件相关验证已执行，或明确说明未执行原因。

## 合入 `master` 前

- [ ] 完整 harness 在干净 checkout 中通过。
- [ ] CMake 核心构建和 Sphinx warning-as-error 构建通过。
- [ ] 没有公开的高置信度密钥、私钥或带认证信息的 URL。
- [ ] 大型测试数据和 Git LFS 对象经过必要性审查。
- [ ] 性能变更提供配置、拓扑、工作负载与重复测量结果。

## 发布前

- [ ] 更新 [`CHANGELOG.md`](https://github.com/promisivia/shm-pcc-sdk/blob/master/CHANGELOG.md) 并确定版本号。
- [ ] 检查 [`THIRD_PARTY_NOTICES.md`](https://github.com/promisivia/shm-pcc-sdk/blob/master/THIRD_PARTY_NOTICES.md)。
- [ ] 确认 [`SECURITY.md`](https://github.com/promisivia/shm-pcc-sdk/blob/master/SECURITY.md) 中的支持范围。
- [ ] 保存 harness JSON 报告并记录硬件验证结果。
- [ ] 检查 GitHub 分支保护、Security Advisories、Issues 与 Discussions 配置。
- [ ] 使用源码归档或 release assets 分发大型镜像，不把它们提交到 Git。

## 周期性维护

- [ ] 更新依赖与 GitHub Actions major versions。
- [ ] 审查第三方代码来源、补丁和许可证变化。
- [ ] 检查文档示例是否仍能从干净 checkout 运行。
- [ ] 复查安全威胁模型、共享内存权限和特权脚本。
- [ ] 清理废弃分支、失效链接和不可复现的性能数据。
