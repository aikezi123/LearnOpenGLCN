# 线程池并发模块

## 1. 本轮基线

本轮线程池学习于 2026-09-03 以当前实现作为阶段性基线。实现位于 `infrastructure/concurrency`，使用 C++17 标准库完成固定工作线程、FIFO 任务队列、条件变量等待、`post()`、通用 `submit()`、`promise/future`、异常传播和排空关闭。

对应生产 target 为：

```text
engineeringlab_concurrency (static library)
    └── Threads::Threads
```

CMake 别名为 `englab::concurrency`。该 target 不依赖 Qt、OpenGL、GLFW、相机 SDK、application 或 domain，可以被外层模块和独立测试直接使用。

当前项目生产代码尚未创建或调用 `ThreadPool`，只有线程池测试直接消费该公共接口。`englab::concurrency` 是独立技术 target，不由聚合 infrastructure target 传播，也不代表 application 已经获得统一异步执行端口。

## 2. 架构边界

`ThreadPool` 是具体并发技术实现，因此放在 infrastructure。当前不提前在 application 增加 `ITaskExecutor`；只有真实 application service 出现“提交后台工作”这一用例需求时，才从该需求抽取最小执行器端口。

未来如果同时存在多个后台执行器，实现与装配关系应为：

```text
application service
    -> ITaskExecutor（application 端口）
            ↑
            ├── ThreadPoolExecutor（infrastructure 实现）
            └── 其他执行策略（infrastructure 或其他外层实现）

composition_root
    -> 选择具体实现、创建实例并注入对应 service
```

不同具体执行器实现 application 端口；组合根创建和注入它们。业务模块不继承 `ThreadPool`，也不直接根据具体执行器类型做分支。

相机控制线程是串行访问设备和维护设备状态的专用执行模型，不应仅因为它也使用队列和 future 就替换成通用线程池。

## 3. 当前公开行为

### 3.1 构造与所有权

- 构造时创建固定数量工作线程；线程数量为 0 时抛出 `std::invalid_argument`。
- `ThreadPool` 不可复制、不可移动。
- 析构函数调用 `shutdown()`，等待工作线程结束。
- 创建工作线程过程中发生异常时，会通知并回收已经创建的线程，然后报告构造失败。

### 3.2 `post()`

```cpp
void post(std::function<void()> task);
```

- 接收无需返回结果的任务。
- 空任务立即抛出 `std::invalid_argument`。
- 关闭开始后拒绝新任务并抛出 `std::runtime_error`，不会静默丢弃。
- 任务在队列锁之外执行。
- `post()`任务抛出的异常不会逃出工作线程；当前实现捕获后不记录，属于已知边界。

### 3.3 `submit()`

```cpp
template<typename F, typename... Args>
auto submit(F&& task, Args&&... taskArgs) -> std::future<ReturnType>;
```

- 使用 `std::invoke_result_t`推导返回类型。
- 使用 `std::decay_t`保存 callable 和参数的值类型。
- 使用 `std::tuple`保存任意数量的参数，并由 `std::apply`在工作线程中展开调用。
- 支持普通返回值和 `void`。
- 支持只能移动的 callable 和参数。
- callable 的异常通过 `std::promise::set_exception()`保存，并由 `future::get()`重新抛出。
- 空 `std::function`可以完成提交，其 `std::bad_function_call`通过 future 传播。
- 包装任务通过 `std::shared_ptr`进入 `std::function<void()>`队列，以兼容内部包含的移动专用对象。

### 3.4 `shutdown()`

当前只提供排空关闭：

```text
停止接受新任务
    -> 唤醒所有工作线程
    -> 执行完已经接受的队列任务
    -> 工作线程退出
    -> join 所有工作线程
```

- 连续多次调用具有幂等行为。
- 多个外部调用线程并发调用会通过关闭互斥量串行化。
- `shutdown()`返回时，关闭前已经接受的任务已经执行完成。
- 当前没有立即取消、丢弃队列、超时关闭或停止令牌。

## 4. 八阶段进度

| 阶段 | 本轮状态 | 当前结论 |
| --- | --- | --- |
| 1. 固定线程 + 任务队列 | 完成 | 固定 worker、FIFO 队列、互斥量和条件变量已实现 |
| 2. 安全关闭与生命周期 | 完成 | 析构排空、线程回收、构造失败清理已实现 |
| 3. 任务返回值 future | 完成 | `promise/future`返回结果已实现 |
| 4. 任意 callable / 参数 / 返回类型 | 完成当前范围 | 通用 `submit()`、多参数、`void`、字符串及移动专用对象已验证 |
| 5. 异常自动传播 | 完成 | `submit()`异常经 future 传播，`post()`异常不杀死 worker |
| 6. 完善 shutdown 语义 | 部分完成 | 已实现排空、幂等、外部并发关闭和关闭后拒绝；更完整状态模型与 worker 内调用边界未定义 |
| 7. 工程健壮性 | 未完成 | 已有基础并发测试，但尚未完成所有严格编译、异常注入和分析器验证 |
| 8. 性能与高级能力 | 未开始 | 尚无有界队列、背压、取消、优先级、动态扩缩容或 work stealing |

阶段“完成当前范围”表示当前教学目标和已列测试通过，不代表覆盖所有 C++ callable 的极端限定形式或所有并发调度组合。

## 5. 测试基线

测试 target 为 `engineeringlab_infrastructure_concurrency_tests`。2026-09-04 在项目命名和基础设施 target 拆分后，使用 `ninja-msvc-debug` 完成配置、全量构建并运行：

```text
20/20 ThreadPoolTest passed
```

当前测试覆盖：

- 构造参数和不可复制/不可移动约束。
- `post()`执行、只执行一次、异常隔离和空任务拒绝。
- `submit()`结果、多参数、`void`、移动专用 callable、移动专用参数、只执行一次和异常传播。
- 空 `std::function`通过 future 传播 `std::bad_function_call`。
- `shutdown()`排空、析构排空、重复调用、外部并发调用和关闭后拒绝。
- 多生产者任务不丢失不重复，以及多个 worker 确实并发执行。

运行方式：

```powershell
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug --target engineeringlab_infrastructure_concurrency_tests
ctest --preset ninja-msvc-debug -R ThreadPoolTest --output-on-failure
```

## 6. 已知边界与后续入口

本轮不继续修改实现，后续重新开启线程池工作时从以下边界继续：

- 明确是否禁止工作线程调用所属线程池的 `shutdown()`，或为其设计不会 self-join 的语义。
- 将单一布尔关闭标志演进为明确的 `Running / ShuttingDown / Stopped`状态时，保持已经接受任务的完成保证。
- 为 fire-and-forget 任务定义异常上报策略；当前异常被隔离但没有日志或回调。
- 增加更强的 submit/shutdown 竞争测试、构造失败注入、Release 和 AddressSanitizer 验证。
- 先通过基准测试确认瓶颈，再考虑有界队列、背压、取消、优先级、动态线程数或 work stealing。
- 等真实 application service 需要后台执行器时，再定义 `ITaskExecutor`并由组合根选择和注入具体实现。
