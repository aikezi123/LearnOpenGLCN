# Debug Notes

本文件记录 Codex 与用户在调试过程中形成的临时结论，避免长对话压缩后丢失背景。

## 当前重要结论

- Qt `QOpenGLWidget` 原型阶段允许先把 Shader、VAO/VBO/EBO、Texture 放在 UI 层跑通。
- 曾经使用 `QImage` / `QOpenGLTexture` 路线显示图片时，关闭窗口后出现 Debug CRT heap corruption。
- 当前更稳定的图片显示路线是 `stb_image + 原生 OpenGL Texture`。
- 相机实时显示后续建议采用：

```text
首次创建纹理：glTexImage2D 分配存储
每帧更新图像：glTexSubImage2D 更新像素数据
```

## 记录规则

- 这里记录“现象、判断、结论”，不要替代代码注释或正式架构文档。
- 如果调试结论变成长期约定，应同步到 `DOC/` 或 `.agents/AGENTS.md`。
