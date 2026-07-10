#include <trajectory/ArchimedeanSpiral2DGenerator.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace learnopengl::domain::trajectory {
namespace {

constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kTinyDistanceMm = 1e-12;

bool nearlyEqual(double lhs, double rhs, double tolerance)
{
    return std::abs(lhs - rhs) <= tolerance;
}

} // namespace

ArchimedeanSpiral2DGenerator::ArchimedeanSpiral2DGenerator(GenerationParameters parameters)
    : m_parameters(parameters)
{
    validateGenerationParameters();
    m_radialGrowthPerRadian = m_parameters.trackSpacingMm / kTwoPi;
}

const ArchimedeanSpiral2DGenerator::GenerationParameters&
ArchimedeanSpiral2DGenerator::generationParameters() const
{
    return m_parameters;
}

const std::vector<ArchimedeanSpiral2DGenerator::Point>&
ArchimedeanSpiral2DGenerator::points() const
{
    return m_points;
}

const std::vector<double>& ArchimedeanSpiral2DGenerator::distancesMm() const
{
    return m_distancesMm;
}

void ArchimedeanSpiral2DGenerator::clearResult()
{
    m_points.clear();
    m_distancesMm.clear();
}

double ArchimedeanSpiral2DGenerator::radialGrowthPerRadian() const
{
    return m_radialGrowthPerRadian;
}

double ArchimedeanSpiral2DGenerator::radiusAtTheta(double thetaRad) const
{
    // r = A + B*theta。
    return m_parameters.startRadiusMm + m_radialGrowthPerRadian * thetaRad;
}

double ArchimedeanSpiral2DGenerator::thetaAtRadius(double radiusMm) const
{
    // θ = (r - A) / B
    if (radiusMm < m_parameters.startRadiusMm - m_parameters.distanceToleranceMm) {
        throw std::invalid_argument("radius must be greater than or equal to startRadiusMm");
    }

    return (radiusMm - m_parameters.startRadiusMm) / m_radialGrowthPerRadian;
}

ArchimedeanSpiral2DGenerator::Point ArchimedeanSpiral2DGenerator::pointAt(double thetaRad, std::size_t rangeIndex) const
{
    // x = rcosθ，y = rsinθ

    // 1.先根据θ算出当前的r
    const double radiusMm = radiusAtTheta(thetaRad);

    Point point;
    // 算出x、y坐标
    point.xMm = radiusMm * std::cos(thetaRad);
    point.yMm = radiusMm * std::sin(thetaRad);
    point.radiusMm = radiusMm;
    point.thetaRad = thetaRad;
    point.rangeIndex = rangeIndex;

    return point;
}

double ArchimedeanSpiral2DGenerator::estimateDeltaTheta(double thetaNowRad, double pointSpacingMm) const
{
    if (pointSpacingMm <= 0.0) {
        throw std::invalid_argument("pointSpacingMm must be greater than 0");
    }
    // 对 r = A + B*theta，局部弧长速度为：
    //     ds/dtheta = sqrt(r(theta)^2 + B^2)
    // 因此 dtheta ~= targetDistance / localSpeed。该值先作为估算；
    // 如果它已经满足误差容限，就不再进入二分迭代。
    const double radiusMm = radiusAtTheta(thetaNowRad);
    const double localSpeed = std::sqrt(radiusMm * radiusMm + m_radialGrowthPerRadian * m_radialGrowthPerRadian + std::numeric_limits<double>::epsilon());

    return pointSpacingMm / localSpeed;
}

double ArchimedeanSpiral2DGenerator::distanceError(double thetaNowRad, double thetaCandidateRad, double pointSpacingMm) const
{
    if (thetaCandidateRad < thetaNowRad) {
        throw std::invalid_argument("thetaCandidateRad must not be less than thetaNowRad");
    }

    // 算出当前弧度对应的坐标点
    const Point now = pointAt(thetaNowRad);
    // 算出候选弧度对应的坐标点
    const Point candidate = pointAt(thetaCandidateRad);
    // 判断两个坐标点的距离差
    return pointDistance(now, candidate) - pointSpacingMm;
}

bool ArchimedeanSpiral2DGenerator::isDistanceErrorWithinTolerance(double errorMm) const
{
    return std::abs(errorMm) <= m_parameters.distanceToleranceMm;
}

ArchimedeanSpiral2DGenerator::DeltaThetaResult ArchimedeanSpiral2DGenerator::solveNextDeltaTheta(double thetaNowRad,
                                                  double thetaEndRad,
                                                  double pointSpacingMm) const
{
    switch (m_parameters.deltaThetaMethod) {
    case DeltaThetaMethod::Bisection:
        return solveNextDeltaThetaBisection(thetaNowRad, thetaEndRad, pointSpacingMm);
    }

    throw std::invalid_argument("unsupported delta theta solve method");
}

// 先尝试局部弧长微分给出的 dtheta 初值；初值不满足容限时，先扩展右边界以夹住
// F(theta) = 实际二维点距 - 目标点距 的零点，再在该有效区间内进行二分求解。
ArchimedeanSpiral2DGenerator::DeltaThetaResult ArchimedeanSpiral2DGenerator::solveNextDeltaThetaBisection(double thetaNowRad,
                                                                                                            double thetaEndRad, double pointSpacingMm) const
{
    // 当前半径段已经没有可向前推进的角度区间。
    if (thetaEndRad <= thetaNowRad) {
        return {};
    }

    // 第 1 步：用局部弧长微分估算 dtheta 初值。
    const double deltaThetaGuess = estimateDeltaTheta(thetaNowRad, pointSpacingMm);
    if (deltaThetaGuess <= 0.0) {
        return {};
    }

    // 第 2 步：先用实际二维点距验证初值；满足容限时无需进入二分法。
    const double thetaEstimated = thetaNowRad + deltaThetaGuess;
    if (thetaEstimated <= thetaEndRad) {
        // 算出估计点与当前点的距离误差
        const double estimatedError = distanceError(thetaNowRad, thetaEstimated, pointSpacingMm);
        // 如果距离误差已经在容限范围之内，直接返回结果。
        if (isDistanceErrorWithinTolerance(estimatedError)) {
            DeltaThetaResult result;
            result.deltaThetaRad = deltaThetaGuess;
            result.nextThetaRad = thetaEstimated;
            result.distanceErrorMm = estimatedError;
            result.iterations = 0;
            result.hasSolution = true;
            result.converged = true;
            return result;
        }
    }

    // 第 3 步：构造二分区间。
    // 左端是当前点，F(thetaLow) 必为负。
    double thetaLow = thetaNowRad;
    // 右端需要满足 F(thetaHigh) >= 0，才能夹住误差函数的零点。
    double thetaHigh = std::min(thetaEndRad, thetaNowRad + 2.0 * deltaThetaGuess);
    // 判断一下右端点F(thetaHigh)
    double highError = distanceError(thetaNowRad, thetaHigh, pointSpacingMm);

    int bracketExpansions = 0;
    // 若右端距离仍不足，则在不超过半径段终点的前提下继续向外扩展右端。
    while (highError < 0.0 && thetaHigh < thetaEndRad) {
        if (bracketExpansions >= m_parameters.maxBracketExpansions) {
            // 未能在允许次数内找到有效右边界，当前半径段无法放下一个完整目标点距。
            return {};
        }

        thetaHigh = std::min(thetaEndRad, thetaHigh + deltaThetaGuess);
        highError = distanceError(thetaNowRad, thetaHigh, pointSpacingMm);
        ++bracketExpansions;
    }

    if (highError < 0.0) {
        // 已到半径段终点仍未达到目标点距；边界点由外层按 appendRangeEndPoint 决定是否追加。
        return {};
    }

    DeltaThetaResult result;
    result.hasSolution = true;

    // 第 4 步：在 F(thetaLow) < 0、F(thetaHigh) >= 0 的有效区间内二分求零点。
    for (int iteration = 1; iteration <= m_parameters.maxIterations; ++iteration) {
        const double thetaMid = 0.5 * (thetaLow + thetaHigh);
        const double midError = distanceError(thetaNowRad, thetaMid, pointSpacingMm);

        result.iterations = iteration;

        if (isDistanceErrorWithinTolerance(midError)) {
            result.nextThetaRad = thetaMid;
            result.deltaThetaRad = thetaMid - thetaNowRad;
            result.distanceErrorMm = midError;
            result.converged = true;
            return result;
        }

        if (midError < 0.0) {
            // 中点距离不足，零点位于右半区，收紧左边界。
            thetaLow = thetaMid;
        } else {
            // 中点距离过大，零点位于左半区，收紧右边界。
            thetaHigh = thetaMid;
        }
    }

    // 达到最大迭代次数后返回当前区间中点，并由 converged 标记其是否仍满足容限。
    const double thetaMid = 0.5 * (thetaLow + thetaHigh);

    result.nextThetaRad = thetaMid;
    result.deltaThetaRad = thetaMid - thetaNowRad;
    result.distanceErrorMm = distanceError(thetaNowRad, thetaMid, pointSpacingMm);
    result.converged = isDistanceErrorWithinTolerance(result.distanceErrorMm);

    return result;
}

void ArchimedeanSpiral2DGenerator::generateInRadiusRange(const RadiusRange& range)
{
    validateRadiusRange(range);

    clearResult();
    appendRadiusRange(range, 0);
    m_distancesMm = computeDistances(m_points);
}

void ArchimedeanSpiral2DGenerator::generate(const std::vector<RadiusRange>& ranges)
{
    validateRadiusRanges(ranges);

    clearResult();

    for (std::size_t rangeIndex = 0; rangeIndex < ranges.size(); ++rangeIndex) {
        appendRadiusRange(ranges[rangeIndex], rangeIndex);
    }

    m_distancesMm = computeDistances(m_points);
}

void ArchimedeanSpiral2DGenerator::validateGenerationParameters() const
{
    if (m_parameters.startRadiusMm < 0.0) {
        throw std::invalid_argument("startRadiusMm must not be negative");
    }

    if (m_parameters.trackSpacingMm <= 0.0) {
        throw std::invalid_argument("trackSpacingMm must be greater than 0");
    }

    if (m_parameters.distanceToleranceMm <= 0.0) {
        throw std::invalid_argument("distanceToleranceMm must be greater than 0");
    }

    if (m_parameters.maxIterations <= 0) {
        throw std::invalid_argument("maxIterations must be greater than 0");
    }

    if (m_parameters.maxBracketExpansions < 0) {
        throw std::invalid_argument("maxBracketExpansions must not be negative");
    }
}

void ArchimedeanSpiral2DGenerator::validateRadiusRange(const RadiusRange& range) const
{
    if (range.startRadiusMm < m_parameters.startRadiusMm - m_parameters.distanceToleranceMm) {
        throw std::invalid_argument("range start radius must not be less than startRadiusMm");
    }

    if (range.endRadiusMm <= range.startRadiusMm) {
        throw std::invalid_argument("range end radius must be greater than range start radius");
    }

    if (range.pointSpacingMm <= 0.0) {
        throw std::invalid_argument("range pointSpacingMm must be greater than 0");
    }
}

void ArchimedeanSpiral2DGenerator::validateRadiusRanges(const std::vector<RadiusRange>& ranges) const
{
    if (ranges.empty()) {
        throw std::invalid_argument("at least one radius range is required");
    }

    for (std::size_t index = 0; index < ranges.size(); ++index) {
        validateRadiusRange(ranges[index]);

        if (index == 0) {
            continue;
        }

        const double expectedStart = ranges[index - 1].endRadiusMm;
        if (!nearlyEqual(ranges[index].startRadiusMm, expectedStart, m_parameters.distanceToleranceMm)) {
            throw std::invalid_argument("radius ranges must be contiguous and sorted");
        }
    }
}

void ArchimedeanSpiral2DGenerator::appendRadiusRange(const RadiusRange& range, std::size_t rangeIndex)
{
    // 将当前半径段的起止半径转换为对应的角度边界。
    const double thetaStart = thetaAtRadius(range.startRadiusMm);
    const double thetaEnd = thetaAtRadius(range.endRadiusMm);

    // 先加入本段首点；相邻半径段共用边界时，避免把相同坐标重复写入 m_points。
    double thetaNow = thetaStart;
    const Point startPoint = pointAt(thetaNow, rangeIndex);
    if (m_points.empty() || pointDistance(m_points.back(), startPoint) > kTinyDistanceMm) {
        m_points.push_back(startPoint);
    }

    // 从当前角度反复求解下一个满足本段目标点间距的角度，并依次追加轨迹点。
    while (thetaNow < thetaEnd) {
        const DeltaThetaResult solveResult = solveNextDeltaTheta(thetaNow, thetaEnd, range.pointSpacingMm);

        if (!solveResult.hasSolution) {
            // 到本段边界前剩余距离不足一个目标点间距，结束正常等点距生成。
            break;
        }

        // 记录本次 dtheta 求解消耗的二分迭代次数，便于后续分析。
        thetaNow = solveResult.nextThetaRad;
        Point nextPoint = pointAt(thetaNow, rangeIndex);
        nextPoint.iterationsFromPreviousPoint = solveResult.iterations;
        m_points.push_back(nextPoint);

        if (thetaEnd - thetaNow <= std::numeric_limits<double>::epsilon()) {
            // 已在浮点精度内到达分段终点，避免继续循环。
            break;
        }
    }

    if (m_parameters.appendRangeEndPoint) {
        // 可选地补充精确落在半径段终点的点；该点与前一点的距离可能小于目标点间距。
        const Point endPoint = pointAt(thetaEnd, rangeIndex);
        if (m_points.empty() || pointDistance(m_points.back(), endPoint) > kTinyDistanceMm) {
            m_points.push_back(endPoint);
        }
    }
}

double ArchimedeanSpiral2DGenerator::pointDistance(const Point& lhs, const Point& rhs)
{
    const double dx = rhs.xMm - lhs.xMm;
    const double dy = rhs.yMm - lhs.yMm;
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<double> ArchimedeanSpiral2DGenerator::computeDistances(const std::vector<Point>& points)
{
    if (points.size() < 2) {
        return {};
    }

    std::vector<double> distances;
    distances.reserve(points.size() - 1);

    for (std::size_t index = 1; index < points.size(); ++index) {
        distances.push_back(pointDistance(points[index - 1], points[index]));
    }

    return distances;
}

} // namespace learnopengl::domain::trajectory
