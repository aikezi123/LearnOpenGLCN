# 相机采集与 OpenGL 显示链路

本文记录当前相机模块已经实现的分层方式、控制线程、任务队列、同步原语和图像帧投递机制。它描述的是当前代码事实，不是一次性完成的最终架构。

本阶段的决定是：相机控制链路继续使用纯 C++17；Qt 只参与 UI 与最终 OpenGL 显示；暂不继续拆分 `DisplayOpenGLImage`。

## 1. 当前阶段目标

相机链路需要同时满足以下约束：

- `domain` 和 `application` 不依赖 Qt 或相机 SDK。
- 相机的打开、关闭、开始采集、停止采集和参数设置必须在一个独立控制线程中串行执行。
- UI 不能直接创建或调用大恒 Galaxy SDK 对象。
- 相机 SDK 回调线程不能直接操作 `QWidget` 或 OpenGL Context。
- 控制命令需要返回设备实际执行后的结果，而不是只返回“已经成功入队”。
- UI 显示速度低于相机出图速度时，旧帧可以丢弃，但不能让 Qt 事件队列无限积压。

当前链路为：

```text
composition_root
    创建 GalaxyCameraController
        ↓ 作为 ICameraDevice 注入
application::CameraCaptureService
        ↓ 作为构造参数注入
ui::CameraImageCaptureView
        ↓ 最终交给
ui::DisplayOpenGLImage
```

编译依赖方向和运行调用方向不是一回事：

```text
编译依赖：
UI ──> Application <── Infrastructure
                         ↑
                  Composition Root 负责选择并装配具体实现

运行调用：
CameraImageCaptureView
    ──> CameraCaptureService
        ──> ICameraDevice 虚函数
            ──> GalaxyCameraController
                ──> Galaxy SDK
```

## 2. 各层职责

### 2.1 Domain：图像帧是什么

`domain::ImageFrame` 只描述一帧图像：

- 宽度和高度；
- 像素格式 `PixelFormat`；
- 帧编号 `frameId`；
- 由 `std::vector<unsigned char>` 独占的连续像素数据。

它不包含 Qt 类型、OpenGL 纹理 ID 或相机 SDK 句柄。因此同一帧既可以来自大恒相机，也可以来自海康相机、文件或测试数据。

### 2.2 Application：相机能力和采集流程

`ICameraDevice` 是 Application 定义的同步端口。它声明 Application 完成相机流程所需要的能力，例如打开设备、开始采集和设置曝光，但不知道这些能力由哪个厂商 SDK 实现。

这里的“同步端口”是指一次 `ICameraDevice` 函数调用会在当前调用线程中执行完设备操作并返回 `CameraResult`。端口自身不创建线程。异步边界由 `CameraCaptureService` 提供：它把同步端口操作封装成命令，交给独立控制线程执行，并向调用者返回 `std::future<CameraResult>`。

`CameraCaptureService` 当前负责：

- 独占一个 `ICameraDevice`；
- 创建并维护一个控制线程；
- 接收和串行执行 FIFO 相机命令；
- 检查操作所需的设备状态；
- 用 `promise/future` 传回实际执行结果；
- 关闭时停止接收命令、排空队列并清理设备。

一个 service 实例只管理一个相机设备。需要同时独立控制多个相机时，应为每台相机分别创建一套 adapter 和 service；是否再增加更高层的多相机协调 service，要由实际同步采集需求决定。

### 2.3 Infrastructure：厂商 SDK 适配

`GalaxyCameraController` 实现 `ICameraDevice`，把通用端口操作翻译成 Galaxy SDK 调用。它使用 Pimpl 将 SDK 头文件、设备句柄、数据流和 SDK 回调类留在 `.cpp` 中，避免厂商类型穿透公共头文件。

### 2.4 UI：交互、跨线程切换和显示

`CameraImageCaptureView` 负责：

- 把按钮和参数控件转换成 Application 请求；
- 展示 `CameraResult`；
- 接收相机帧并将它安全切换到 Qt UI 线程；
- 只保留尚未显示的最新一帧；
- 把最终帧交给 `DisplayOpenGLImage`。

`DisplayOpenGLImage` 当前仍是 UI 原型控件，直接管理 Shader、VAO/VBO/EBO、Texture、纹理上传和观察变换。本阶段明确不拆分它，后续只有在相机控制链路稳定后再单独处理 OpenGL 资源职责。

### 2.5 Composition Root：对象创建和依赖注入

`CameraComposition` 创建 `GalaxyCameraController`，将它按 `ICameraDevice` 类型交给 `CameraCaptureService`，再把 service 交给 `CameraImageCaptureView`。UI 因此只依赖 Application，不需要包含 Galaxy 适配器。

对象所有权依次转移：

```text
CameraImageCaptureView
    owns unique_ptr<CameraCaptureService>
        控制线程的 run() 参数
            owns unique_ptr<ICameraDevice>
                实际对象是 GalaxyCameraController
```

设备所有权被移动到 `run()` 的局部参数中，所以相机操作和设备最终析构都发生在控制线程。

## 3. 控制线程的生命周期

`CameraCaptureService` 构造时启动控制线程：

```cpp
m_thread = std::thread(
    &CameraCaptureService::run,
    this,
    std::move(cameraDevice)
);
```

四个参数角色分别是：

- `std::thread(...)`：创建线程；
- `&CameraCaptureService::run`：线程入口成员函数；
- `this`：调用该成员函数所需的对象；
- `std::move(cameraDevice)`：移动到 `run()` 参数中的设备所有权。

线程启动后不会马上销毁设备。`run()` 是一个长期存在的循环：没有命令时睡眠，有命令时执行，收到退出请求且队列为空时才离开循环。

```text
构造 service
    ↓
启动控制线程并进入 run()
    ↓
等待命令 ──> 取出一条命令 ──> 执行 ──┐
    ↑                                  │
    └──────────────────────────────────┘
    ↓ shutdown 已请求且队列为空
注销帧回调 -> 停止采集 -> 关闭设备
    ↓
run() 返回，设备在控制线程析构
    ↓
shutdown() 中的 join() 返回
```

## 4. 命令和 FIFO 任务队列

当前命令动作只有一层函数包装：

```cpp
using CommandAction =
    std::function<CameraResult(ICameraDevice& device)>;

struct Command {
    CommandAction action;
    std::promise<CameraResult> promiseResult;
};
```

`action` 描述“要对设备做什么”，参数使用 `ICameraDevice&`，因此同一套队列机制不依赖 Galaxy 的具体类型。`promiseResult` 对应这条命令最终的完成结果。

队列使用 `std::queue<Command>`，对外只暴露 FIFO 所需的 `push`、`front` 和 `pop`，能直接表达“先投递、先执行”。`std::queue` 本身是容器适配器，其默认底层通常是 `std::deque`，但 service 不依赖底层容器的其他操作。

一次请求的投递过程如下：

```text
调用 requestSetFps(30.0)
    ↓
构造捕获参数的 CommandAction
    ↓
submit(action)
    ├── 创建 Command
    ├── promise.get_future()
    ├── 在 m_mutex 保护下 push 到 m_commands
    ├── condition_variable.notify_one()
    └── 立即把 future 返回给调用者
```

控制线程的处理过程如下：

```text
wait() 返回
    ↓
在锁内移动队首命令并 pop
    ↓
释放 m_mutex
    ↓
command.action(*cameraDevice)
    ↓
command.promiseResult.set_value(result)
```

命令必须在释放 `m_mutex` 后执行。相机 SDK 操作可能耗时，如果执行期间仍持有队列锁，其他线程就无法继续提交命令，`shutdown()` 也无法设置退出标志。

### 4.1 连续投递多条命令时会不会丢失唤醒

不会。条件变量的通知只负责提示“状态可能变化了”，真正的任务保存在队列中。

如果控制线程正在执行一个耗时命令，调用线程又连续投递三条命令，那么这些命令会依次留在 `m_commands` 中。期间的多个 `notify_one()` 可能合并成一次有效唤醒，但当前命令结束后，控制线程再次检查谓词时会看到队列非空，因此不会睡眠，而是继续取下一条命令。

所以这里可靠的依据是：

```text
通知可能合并，但队列状态不会因通知合并而消失。
```

当前循环一次只取一条命令，使锁的持有时间很短。随后循环会继续排空已有命令，并不要求每条命令都对应一次独立唤醒。

## 5. 条件变量的实现机制

等待代码为：

```cpp
std::unique_lock<std::mutex> lock(m_mutex);
m_condition.wait(lock, [this]() {
    return m_shutdownRequest || !m_commands.empty();
});
```

`wait(lock, predicate)` 的两个参数含义是：

1. `lock` 是一个已经锁住 `m_mutex` 的 `std::unique_lock`。条件变量等待期间需要临时解锁和再次加锁，因此不能使用不能手动解锁/加锁的 `std::lock_guard`。
2. `predicate` 是可调用对象。这里只使用 lambda；它返回 `bool`，表示“线程现在是否已经具备继续运行的条件”。

其行为可以近似理解为：

```cpp
while (!predicate()) {
    // 原子地释放 mutex 并进入睡眠
    wait_for_notification();
    // 被唤醒后重新锁住 mutex，再检查 predicate
}
```

重要结论：

- 条件变量不保存命令，也不保存业务状态。
- 条件变量并没有永久“绑定”某一把锁；是每次调用 `wait` 时传入锁，但所有参与者必须按同一约定用同一互斥量保护谓词读取的数据。
- `notify_one()` 不是命令，只是唤醒一个等待者重新检查条件。
- 线程可能发生虚假唤醒，所以不能只写无谓词的 `wait(lock)` 后就假定一定有任务。
- 谓词为 `true` 时 `wait` 才返回给后续代码；如果被唤醒后仍为 `false`，它会继续等待。

当前谓词读取 `m_shutdownRequest` 和 `m_commands.empty()`，这两份数据都由 `m_mutex` 保护。

条件变量和 Qt 信号槽在“通知另一个执行上下文”这一点上看起来相似，但职责不同：条件变量是底层线程同步工具，不携带业务参数、不选择槽函数，也不自动跨线程排队执行对象方法；本项目的命令对象和 FIFO 队列才承担了类似“排队调用”的部分。

## 6. 每把锁具体保护什么

锁保护的是共享数据的不变量，不是笼统地“保护一个线程”。当前相机链路中的锁如下：

| 位置 | 锁 | 保护的数据或约束 | 不应在锁内做的工作 |
| --- | --- | --- | --- |
| `CameraCaptureService` | `m_mutex` | `m_shutdownRequest`、`m_commands`，以及二者联合判断 | 相机 SDK 操作、等待 `join()` |
| `GalaxyCameraControllerImpl` | `m_callbackMutex` | `m_frameCallback` 的替换、清空和调用互斥 | 长时间 UI 或设备控制操作 |
| `GalaxyCameraControllerImpl` | `m_errorMutex` | `m_lastError` 的读写 | 相机 SDK 操作 |
| `CameraImageCaptureView` | `m_latestFrameMutex` | `m_latestFrame` 与 `m_frameDisplayPending` 必须作为一组保持一致 | OpenGL 上传、Qt 控件更新 |

`m_callbackMutex` 在调用帧回调期间仍保持锁定，因此 `setFrameCallback({})` 返回时，可以确认已经进入的旧回调执行完毕。这对 UI 析构很重要：页面先同步注销回调，随后才销毁自身，避免 SDK 线程继续访问已经释放的 `this`。

这种约束也要求当前帧回调保持短小。它只把最新帧移动到 UI 邮箱并投递 Qt 任务，不能在里面等待新的相机控制命令，否则可能形成锁等待环。

## 7. 原子状态的实现机制

`CameraCaptureService` 使用：

```cpp
std::atomic<State> m_state{State::Closed};
```

它解决的是控制线程写状态、UI 线程读状态时的数据竞争：

- 相机命令只在控制线程执行，因此状态转换也只由控制线程写入；
- UI 可以通过 `state()` 读取最近一次已经完成的状态快照；
- 读写单个枚举不需要为了线程安全再占用命令队列锁。

当前状态语义为：

| 枚举值 | 当前含义 |
| --- | --- |
| `Closed` | 设备没有打开 |
| `Opened` | 设备已打开，但没有采集 |
| `Captured` | 正在采集图像；名称保留自当前代码，语义相当于 `Capturing` |

原子变量只保证这一个状态值的并发读写安全，不保证 UI 读取状态后，设备在下一刻仍保持不变，也不把多个对象操作组合成事务。因此真正的状态检查仍放在控制线程命令内部，UI 按钮禁用只用于改善交互，不能代替 Application 校验。

当前 `load()` / `store()` 没有显式传入内存序，使用标准库默认的 `std::memory_order_seq_cst`。对当前简单状态快照来说这是正确且容易理解的选择，不需要为了优化过早改用更弱的内存序。

Galaxy adapter 中的 `m_isOpen` 和 `m_isGrabbing` 也使用原子布尔值，用于让 SDK 回调/清理路径安全观察设备状态；它们不能替代控制线程对设备操作的串行化。

## 8. promise + future 的实现机制

`std::promise<T>` 和 `std::future<T>` 是同一份共享状态的写入端与读取端：

```text
控制线程持有 promise ── set_value(result) ──┐
                                              ├── 共享状态
调用线程持有 future  ─────── get() <─────────┘
```

在 `submit()` 中，必须先调用一次 `promise.get_future()` 建立读取端，然后再把包含 promise 的 `Command` 移进队列：

```cpp
Command command;
std::future<CameraResult> future =
    command.promiseResult.get_future();

m_commands.push(std::move(command));
return future;
```

`std::promise` 不可复制但可以移动，所以 `Command` 也会随之成为只能移动的命令对象。`std::queue::push(std::move(command))` 将 promise 的写入端连同命令一起交给控制线程；已经返回的 future 仍连接同一共享状态。

控制线程执行设备操作后：

```cpp
command.promiseResult.set_value(std::move(result));
```

此时 future 变为 ready：

- 如果调用者尚未调用 `get()`，结果保存在共享状态中等待读取；
- 如果调用者已经在 `get()` 中等待，它会被唤醒并取得结果；
- 每个 future 只能通过 `get()` 消费一次；
- 同一个 promise 也只能完成一次。

### 8.1 为什么它是异步机制，而当前 UI 看起来仍是同步的

请求函数返回 future 时，命令通常还只是在队列中，真正的设备操作会在控制线程执行，因此 service 接口具备异步结果传递能力。

但当前 UI 紧接着调用 `.get()`：

```cpp
CameraResult result =
    m_cameraCaptureService->requestOpen().get();
```

这会阻塞 Qt UI 线程，直到控制线程完成打开操作。也就是说：

```text
操作执行线程：控制线程，已经与 UI 分离
当前结果等待方式：UI 线程同步等待
```

这是本阶段为降低 Qt 侧复杂度保留的简化。它不会让 SDK 操作跑回 UI 线程，但耗时命令仍可能造成界面短暂无响应。后续可以在不改变 Application 纯 C++ 接口的前提下，由 UI 外层使用定时检查、等待线程或 Qt Concurrent 观察 future，再通过 queued invoke 更新界面。

### 8.2 future 的完成路径和异常安全

当前代码保证常规控制流程中的命令都有明确结果：

- 空命令在 `submit()` 中立即写入失败结果；
- shutdown 后提交的命令立即写入失败结果；
- 状态不允许的操作由命令返回失败结果；
- 设备操作返回的 `CameraResult` 被写入 promise；
- 命令动作抛出的 `std::exception` 或未知异常会被转换成失败的 `CameraResult`，随后写入 promise；
- shutdown 前已入队的命令会先被排空，再退出控制线程。

捕获异常的目的不只是防止控制线程退出，还为了避免 promise 在未完成时被析构。未完成的 promise 被销毁时，future 会得到 `std::future_error` 的 `broken_promise`，而不是项目统一的 `CameraResult`。

`set_value()` 按规则也可能抛出 `std::future_error`，例如同一个 promise 被重复完成。当前实现通过“每个命令只在 run() 的一个位置完成一次”和“每个 promise 只调用一次 get_future()”维持这一不变量，因此正常路径不会触发该异常。以后如果增加取消、超时或多处完成逻辑，必须重新审查并保证每条命令只完成一次。

## 9. shutdown 的顺序和边界

`shutdown()` 当前执行：

```text
锁住 m_mutex
    ↓
m_shutdownRequest = true
    ↓
解锁并 notify_one()
    ↓
join() 等待控制线程退出
```

控制线程不会看到 shutdown 就立刻丢弃已接受的任务。退出条件是：

```cpp
m_shutdownRequest && m_commands.empty()
```

因此顺序为：停止接收新命令，继续执行已入队命令，队列为空后注销帧回调、停止采集并关闭设备，最后让设备在控制线程中析构。

`join()` 不能放在 `m_mutex` 的临界区内，否则控制线程为了重新取得同一把锁检查退出条件而等待，调用线程又在等待控制线程结束，会形成死锁。

当前实现有一个明确使用约束：service 的生命周期所有者负责调用 shutdown，代码没有额外的 `m_shutdownMutex`，因此不支持多个外部线程并发调用 `shutdown()`。析构函数会再次调用 `shutdown()`，但在先前调用已经完成的顺序场景中，`m_thread.joinable()` 为 false，不会重复 join。

## 10. 相机出图到 UI 的最新帧机制

控制命令和图像帧走的是两条不同通道：

```text
控制通道：
Qt UI -> CameraCaptureService 队列 -> 控制线程 -> Galaxy SDK

图像通道：
Galaxy SDK 采集线程
    -> GalaxyCameraController::handleFrame()
    -> RGB24 ImageFrame
    -> ICameraDevice::FrameCallback
    -> CameraImageCaptureView::submitLatestFrame()
    -> Qt UI 线程
    -> DisplayOpenGLImage
```

Galaxy adapter 将 SDK 图像转换成 RGB24，并复制到 `ImageFrame::pixels`。从这一刻开始，像素内存由标准 C++ 容器拥有，不再依赖 SDK 缓冲区寿命。

SDK 线程不能直接调用 `DisplayOpenGLImage`，因为 QWidget 和 OpenGL Context 属于 UI 线程。`CameraImageCaptureView` 使用一个“单槽最新帧邮箱”：

```cpp
std::mutex m_latestFrameMutex;
domain::ImageFrame m_latestFrame;
bool m_frameDisplayPending{false};
```

每次到帧时：

1. 在 `m_latestFrameMutex` 下用新帧覆盖 `m_latestFrame`。
2. 如果尚未向 Qt 事件队列投递显示任务，则把 `m_frameDisplayPending` 设为 true 并投递一次 queued invoke。
3. 如果已经有显示任务等待执行，只覆盖帧，不继续添加 Qt 事件。
4. UI 任务执行时移动取出最新帧，并在同一把锁下将 pending 设回 false。
5. 释放锁后调用 `DisplayOpenGLImage::setRgb24Frame()`。

这一策略表达的是实时预览语义：宁可丢弃已经过时的中间帧，也要显示尽可能新的帧并保持有限内存。它不适用于“每一帧都必须处理”的录像、测量或离线算法；这类消费者以后应使用独立的有界队列和明确的背压/丢帧策略。

## 11. 当前实现与后续边界

本阶段已经完成：

- 相机模型、Application 端口、Galaxy adapter、UI 和组合根的最小分层；
- 纯 C++17 独立控制线程；
- FIFO 命令队列、条件变量和状态校验；
- `promise/future` 异步结果通道；
- 命令异常转换和 shutdown 前队列排空；
- 按首台设备、ID、名称打开，以及开始/停止/关闭；
- 自动白平衡、曝光、增益和帧率设置；
- SDK 线程到 Qt UI 线程的最新帧覆盖投递。

本阶段刻意保留：

- UI 对 future 立即调用 `.get()`，耗时控制命令仍可能短暂阻塞 UI；
- `State::Captured` 的现有命名；
- shutdown 由单一生命周期所有者调用，不支持并发 shutdown；
- `DisplayOpenGLImage` 继续承担现有 OpenGL 显示职责，不在本阶段拆分；
- 尚未建立自动化的相机 service 单元测试和模拟设备测试。

后续重构应按独立阶段推进，不应把这些工作一次性混入当前稳定链路。优先候选包括：

1. 为 `CameraCaptureService` 增加不依赖真实 SDK 的 fake device 测试，验证 FIFO、状态转换、异常和 shutdown。
2. 让 UI 非阻塞地观察 future，保持 Application 接口仍为纯 C++。
3. 根据真实需求补充设备枚举、能力查询和参数范围，而不是预先扩张端口。
4. 最后再评估 `DisplayOpenGLImage` 的 UI 交互与 OpenGL 资源职责拆分。

## 12. 代码阅读入口

- 图像模型：`domain/image/include/imageframe/ImageFrame.h`
- 相机端口：`application/camera/include/camera/ICameraDevice.h`
- 控制 service：`application/camera/include/camera/CameraCaptureService.h`
- 控制线程实现：`application/camera/src/CameraCaptureService.cpp`
- Galaxy adapter：`infrastructure/camera/galaxy/include/camera/galaxy/GalaxyCameraController.h` 与对应 `.cpp`
- 相机页面：`ui/include/CameraImageCaptureView.h` 与对应 `.cpp`
- OpenGL 显示控件：`ui/include/DisplayOpenGLImage.h` 与对应 `.cpp`
- 相机装配：`composition_root/modules/CameraComposition.cpp`
