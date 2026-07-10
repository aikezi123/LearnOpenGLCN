# 二维阿基米德螺旋轨迹

## 1. 当前实现范围

当前实现位于 `domain/trajectory`：

- `ArchimedeanSpiral2DGenerator` 是不依赖 Qt、OpenGL、文件系统的二维纯算法。
- 只生成 XOY 平面点，不计算 Z、高度曲面或三维点间距。
- 轨迹固定为圆形阿基米德螺旋；线间距在整条轨迹上固定，半径分段只允许改变目标点间距。

算法说明见公开头文件 `domain/trajectory/include/trajectory/ArchimedeanSpiral2DGenerator.h`，实现见对应 `.cpp` 文件。

## 2. 轨迹模型与单位

轨迹方程为：

```text
r(theta) = A + B * theta
B = trackSpacingMm / (2 * pi)
x = r * cos(theta)
y = r * sin(theta)
```

其中：

- `A` 是 `startRadiusMm`，单位 mm。
- `trackSpacingMm` 是相邻完整螺旋圈的径向线间距，单位 mm。
- `B` 是每弧度的半径增长量，单位 mm/rad。
- `theta`、`dtheta` 使用 rad。
- 所有点距、残差和半径范围使用 mm。

因此角度每增加 `2*pi`，半径严格增加一次 `trackSpacingMm`。

## 3. 参数与结果

`GenerationParameters` 保存整条螺旋线和求解器的公共参数：起始半径、线间距、距离残差容限、最大二分次数、右边界扩展次数、求解方法以及是否追加精确分段终点。

`RadiusRange` 定义单个半径段：

```cpp
{ startRadiusMm, endRadiusMm, pointSpacingMm }
```

多个半径段必须按半径升序连续连接。`Point` 保存 `xMm`、`yMm`、`radiusMm`、`thetaRad`、`rangeIndex` 和从前一点求解到本点所用的迭代次数。

`rangeIndex` 只表示点所属的半径段编号，不参与几何计算。

## 4. 点间距求解

对圆形阿基米德螺旋，局部弧长速度为：

```text
ds / dtheta = sqrt(r^2 + B^2)
```

`estimateDeltaTheta()` 先给出初值：

```text
dthetaGuess = pointSpacingMm / sqrt(r^2 + B^2)
```

最终要求满足的是相邻点的二维直线距离，而不是局部弧长。定义带符号距离残差：

```text
F(thetaCandidate)
  = distance(point(thetaCandidate), point(thetaNow))
    - pointSpacingMm
```

- `F < 0`：候选点距离不足。
- `F = 0`：候选点满足目标点距。
- `F > 0`：候选点距离过大。

`solveNextDeltaTheta()` 按 `DeltaThetaMethod` 分派具体求解器。当前只实现二分法：

1. 验证局部弧长初值；满足容限时直接返回，迭代次数为 `0`。
2. 以当前角度作为左边界，保证 `F(thetaLow) < 0`。
3. 使用约两倍初值建立右边界；若右端仍不足则逐步扩展，且不超过当前分段终点。
4. 在 `F(thetaLow) < 0`、`F(thetaHigh) >= 0` 的有效区间内二分，直到残差满足容限或达到最大迭代次数。

达到最大迭代次数后，求解器返回当前区间中点，并通过 `converged` 标记是否满足容限。当前生成循环以 `hasSolution` 决定是否追加该点，因此需要在后续质量分析中同时关注 `converged`。

## 5. 分段与边界行为

`generate(ranges)` 会清空旧结果，再按顺序将所有分段追加到 `m_points`。`m_points` 因而保存最近一次多段生成后的完整点序列；每个点可通过 `rangeIndex` 追溯来源。

相邻分段共享的边界点会去重。正常等点距生成在剩余长度不足一个目标点距时停止。

当 `appendRangeEndPoint = true` 时，生成器会额外补充精确落在 `endRadiusMm` 的边界点。该补点与前一点的距离可能小于本段目标点距；这是确保半径边界精确的设计结果，不是求解器的距离残差失效。

`generateInRadiusRange(range)` 也会清空旧结果，但只生成一个分段，且该段内点的 `rangeIndex` 为 `0`。

## 6. UI 与导出

Qt 主窗口在“轨迹算法 / 螺旋线导出”页面提供参数设置、半径分段编辑和 txt 导出。导出任务使用 Qt Concurrent 在后台执行，导出期间禁用编辑控件，关闭页面时请求任务停止并等待任务结束，避免后台任务访问已经销毁的 UI 对象。

每次导出会写出：

- 合并的 `archimedean_spiral_xy.txt` 与 `archimedean_spiral_iterations.txt`。
- 每个半径段独立的 `archimedean_spiral_range_<n>_xy.txt` 与对应迭代次数文件。

后台日志和页面状态会分别显示每段的轨迹生成耗时，以及汇总的轨迹生成耗时、文件写入耗时和导出总耗时。算法耗时仅包围 `generateInRadiusRange()`，不包括 txt 写入。

`tools/trajectory` 还保留一个可独立配置和编译的 C++ txt 导出工具；它不是主工程顶层 CMake 构建的一部分。

## 7. 后续扩展边界

椭圆螺旋不能直接复用圆形模型中的 `sqrt(r^2 + B^2)` 局部速度公式。若引入椭圆模型，需要同时替换 `pointAt()` 和局部速度计算；基于实际二维点距残差的夹逼与二分框架可以继续复用。

三维轨迹应在明确曲面模型后计算 `z`。不能把二维点序号的线性递增直接当作曲面高度，否则 Z 变化会随点数和点间距改变。文件导出、三维曲面映射和 OpenGL 渲染仍属于 domain 之外的后续工作。
