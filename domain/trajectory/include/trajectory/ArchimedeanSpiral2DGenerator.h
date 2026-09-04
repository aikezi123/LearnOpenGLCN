#pragma once

#include <cstddef>
#include <vector>

namespace engineeringlab::domain::trajectory {

// 固定阿基米德螺旋线二维采样点生成器。
//
// 轨迹方程固定为：
//
//     r(theta) = A + B * theta
//     B = trackSpacing / (2*pi)
//
// 线间距 trackSpacing 决定整条螺旋线的形状，不随半径段变化；
// 不同半径段只改变该段的目标点间距 pointSpacing。
// 当前只控制 XOY 平面点距，暂不把 Z 方向曲面高度变化纳入点距计算。
class ArchimedeanSpiral2DGenerator final {
public:
    // 求解下一个角度步长 dtheta 的方法。
    // 当前只实现二分法，后续可以继续扩展牛顿法、割线法等。
    enum class DeltaThetaMethod {
        Bisection
    };

    // 整条螺旋线和求解器的全局参数。
    // 半径段内的点间距由 RadiusRange 指定，不放在这里。
    struct GenerationParameters final {
        double startRadiusMm{0.0};                                      // r = A + B*theta 中的 A，单位 mm。
        double trackSpacingMm{0.004};                                   // 相邻两圈螺旋线的径向距离，单位 mm；B 由它计算。
        double distanceToleranceMm{1e-6};                                // 距离误差函数 F(theta) 的绝对误差容限，单位 mm。
        int maxIterations{30};                                           // 找到有效左右边界后，二分法最多迭代次数。
        int maxBracketExpansions{32};                                    // 寻找二分法右边界时，允许扩展右边界的最大次数。
        DeltaThetaMethod deltaThetaMethod{DeltaThetaMethod::Bisection};  // 当前使用的 dtheta 求解方法。
        bool appendRangeEndPoint{true};                                  // 是否追加精确落在 endRadiusMm 上的半径段边界点。
    };

    // 单个半径范围段。轨迹方程仍由 GenerationParameters 固定；
    // 该结构只定义本段半径范围和本段目标点间距。
    struct RadiusRange final {
        double startRadiusMm{0.0};     // 起始半径，单位 mm。
        double endRadiusMm{0.0};       // 结束半径，单位 mm，必须大于 startRadiusMm。
        double pointSpacingMm{0.003};  // 当前半径段内的目标点间距，单位 mm。
    };

    // 生成出的单个二维轨迹点。
    struct Point final {
        double xMm{0.0};                         // x 坐标，单位 mm。
        double yMm{0.0};                         // y 坐标，单位 mm。
        double radiusMm{0.0};                    // 当前点半径，单位 mm。
        double thetaRad{0.0};                    // 当前点极角，单位 rad。
        std::size_t rangeIndex{0};               // 当前点所属半径段索引。例如有3段，第一段半径ranges[0] = {0.0, 0.3, 0.005};ranges[1] = {0.3, 1.0, 0.010};ranges[2] = {1.0, 3.5, 0.050};
        int iterationsFromPreviousPoint{0};      // 从上一个点求解到当前点时消耗的迭代次数；直接估算满足容限时为 0。
    };

    // 单次 dtheta 求解结果。
    // 该结构只描述“求下一个点”这一小步，不作为整条轨迹的输出结果。
    struct DeltaThetaResult final {
        double deltaThetaRad{0.0};   // 求得的角度步长，单位 rad。
        double nextThetaRad{0.0};    // 下一个点的 theta，单位 rad。
        double distanceErrorMm{0.0}; // F(theta) = 实际二维点距 - 目标点距，单位 mm。
        int iterations{0};           // 本次求解消耗的迭代次数；直接估算满足容限时为 0。
        bool hasSolution{false};     // 是否找到了可用的下一个点。
        bool converged{false};       // 求解结果是否满足 distanceToleranceMm。
    };

public:
    // 常规使用接口：配置生成器、生成轨迹、读取最近一次生成结果。
    explicit ArchimedeanSpiral2DGenerator(GenerationParameters parameters);

    const GenerationParameters& generationParameters() const;

    // 在单个半径范围内生成轨迹，并刷新成员变量中的结果。
    void generateInRadiusRange(const RadiusRange& range);

    // 在多个连续半径范围内生成轨迹，并刷新成员变量中的结果。
    void generate(const std::vector<RadiusRange>& ranges);

    // 最近一次生成得到的轨迹点。
    const std::vector<Point>& points() const;

    // 最近一次生成得到的相邻点二维距离。
    const std::vector<double>& distancesMm() const;

    // 清空最近一次生成结果，不改变螺旋线参数。
    void clearResult();

public:
    // 算法辅助接口：用于轨迹检查、误差分析和后续求解方法扩展。
    // 返回 r = A + B*theta 中的 B。
    double radialGrowthPerRadian() const;

    // 根据 theta 计算半径 r。即求r = A + B*theta。
    double radiusAtTheta(double thetaRad) const;

    // 根据半径反算 theta。θ = (r - A) / B
    double thetaAtRadius(double radiusMm) const;

    // 根据 theta 计算二维点。x = rcosθ，y = rsinθ
    Point pointAt(double thetaRad, std::size_t rangeIndex = 0) const;

    // 使用局部弧长微分估算 dtheta 初值。dθ = pointSpacing / sqrt(r^2 + B^2)。
    double estimateDeltaTheta(double thetaNowRad, double pointSpacingMm) const;

    // 两点之间距离与点间距之间的距离误差。F(thetaCandidate) = distance(point(thetaCandidate), point(thetaNow)) - pointSpacingMm
    double distanceError(double thetaNowRad, double thetaCandidateRad, double pointSpacingMm) const;

    // 判断距离误差是否已经满足当前容限。
    bool isDistanceErrorWithinTolerance(double errorMm) const;

    // 按 deltaThetaMethod 求解下一个 dtheta。
    DeltaThetaResult solveNextDeltaTheta(double thetaNowRad, double thetaEndRad, double pointSpacingMm) const;

    // 使用二分法求解下一个 dtheta。目标找到下一个θnext,使得|distance(P(θnow), P(θnext)) - pointSpacing| ≤ ε。dθ = θnext - θnow
    // thetaNowRad是当前轨迹点的极角。thetaEndRad是当前半径分段结束边界对应的极角，单位都是rad。
    DeltaThetaResult solveNextDeltaThetaBisection(double thetaNowRad, double thetaEndRad, double pointSpacingMm) const;

private:
    void validateGenerationParameters() const; // 校验起始半径、线间距、误差容限和求解器次数等全局参数。
    void validateRadiusRange(const RadiusRange& range) const; // 校验单个半径段的范围和目标点间距。
    void validateRadiusRanges(const std::vector<RadiusRange>& ranges) const; // 校验多个半径段非空、连续且顺序正确。

    void appendRadiusRange(const RadiusRange& range, std::size_t rangeIndex); // 将指定半径段生成的点追加到 m_points。

    static double pointDistance(const Point& lhs, const Point& rhs); // 计算两个轨迹点在 XOY 平面内的欧氏距离，单位 mm。
    static std::vector<double> computeDistances(const std::vector<Point>& points); // 计算输入点序列中所有相邻点的二维距离。

    GenerationParameters m_parameters;          // 当前生成器使用的全局配置参数副本。
    double m_radialGrowthPerRadian{0.0};         // r = A + B*theta 中的 B，单位 mm/rad。
    std::vector<Point> m_points;                 // 最近一次生成得到的全部二维轨迹点。
    std::vector<double> m_distancesMm;           // 最近一次结果中相邻点的二维距离，数量通常为 m_points.size()-1。
};

} // namespace engineeringlab::domain::trajectory
