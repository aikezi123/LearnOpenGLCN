# 测试目录只负责组织测试 target，不向生产模块传播任何依赖。
# 当前阶段先提供框架，具体测试文件和 target 后续按模块逐步加入。

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/LearnOpenGLAddTest.cmake")

# 后续推荐按生产模块镜像组织：
#
# tests/
# ├── domain/
# ├── application/
# └── infrastructure/
#
# 模块测试目录创建后在这里使用 add_subdirectory(...) 加入。
