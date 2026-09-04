# EngineeringLab Agent 入口

本文件是仓库根目录的自动发现入口，适用于整个项目。详细协作规则与当前状态分别维护在 `.agents/AGENTS.md` 和 `.agents/CODEX_CONTEXT.md`。

开始工作前按顺序读取：

1. `.agents/AGENTS.md`
2. `.agents/CODEX_CONTEXT.md`
3. `README.md`
4. 与当前任务相关的 `DOC/` 文档、CMake 文件和源码

文档职责：

- `README.md` 是项目说明与正式文档的统一入口。
- `DOC/` 保存架构、模块设计、开发规范和测试说明正文。
- `.agents/AGENTS.md` 只记录 Agent 执行工作时必须遵守的规则。
- `.agents/CODEX_CONTEXT.md` 只记录当前阶段快照、重要决策和继续工作的入口。

不要在 `.agents/` 中复制完整的模块文档。若上下文记录与代码、CMake 或 `DOC/` 的当前事实不一致，应核对实现并同步修正文档。

若本入口与 `.agents/AGENTS.md` 冲突，以 `.agents/AGENTS.md` 的详细规则为准。
