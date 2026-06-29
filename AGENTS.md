# LearnOpenGLCN Codex 入口

本文件是 Codex 自动读取的根目录入口。详细项目协作规则已经移动到 `.agents/AGENTS.md`，上下文记录移动到 `.agents/CODEX_CONTEXT.md`。

以后 Codex 在本项目中工作时必须先读取：

1. `.agents/AGENTS.md`
2. `.agents/CODEX_CONTEXT.md`
3. 与当前任务相关的 `DOC/` 文档、CMake 文件和源码文件

根目录只保留本入口文件，避免 Codex/Agent 相关文档散落在项目根目录。

若本文件与 `.agents/AGENTS.md` 冲突，以 `.agents/AGENTS.md` 的详细规则为准。
