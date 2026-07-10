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
    return m_parameters.startRadiusMm + m_radialGrowthPerRadian * thetaRad;
}

double ArchimedeanSpiral2DGenerator::thetaAtRadius(double radiusMm) const
{
    if (radiusMm < m_parameters.startRadiusMm - m_parameters.distanceToleranceMm) {
        throw std::invalid_argument("radius must be greater than or equal to startRadiusMm");
    }

    return (radiusMm - m_parameters.startRadiusMm) / m_radialGrowthPerRadian;
}

ArchimedeanSpiral2DGenerator::Point
ArchimedeanSpiral2DGenerator::pointAt(double thetaRad, std::size_t rangeIndex) const
{
    const double radiusMm = radiusAtTheta(thetaRad);

    Point point;
    point.xMm = radiusMm * std::cos(thetaRad);
    point.yMm = radiusMm * std::sin(thetaRad);
    point.radiusMm = radiusMm;
    point.thetaRad = thetaRad;
    point.rangeIndex = rangeIndex;

    return point;
}

double ArchimedeanSpiral2DGenerator::estimateDeltaTheta(double thetaNowRad,
                                                        double pointSpacingMm) const
{
    if (pointSpacingMm <= 0.0) {
        throw std::invalid_argument("pointSpacingMm must be greater than 0");
    }

    // 对 r = A + B*theta，局部弧长速度为：
    //
    //     ds/dtheta = sqrt(r(theta)^2 + B^2)
    //
    // 因此 dtheta ~= targetDistance / localSpeed。该值先作为估算；
    // 如果它已经满足误差容限，就不再进入二分迭代。
    const double radiusMm = radiusAtTheta(thetaNowRad);
    const double localSpeed = std::sqrt(radiusMm * radiusMm
                                        + m_radialGrowthPerRadian * m_radialGrowthPerRadian
                                        + std::numeric_limits<double>::epsilon());

    return pointSpacingMm / localSpeed;
}

double ArchimedeanSpiral2DGenerator::distanceError(double thetaNowRad,
                                                   double thetaCandidateRad,
                                                   double pointSpacingMm) const
{
    if (thetaCandidateRad < thetaNowRad) {
        throw std::invalid_argument("thetaCandidateRad must not be less than thetaNowRad");
    }

    const Point now = pointAt(thetaNowRad);
    const Point candidate = pointAt(thetaCandidateRad);

    return pointDistance(now, candidate) - pointSpacingMm;
}

bool ArchimedeanSpiral2DGenerator::isDistanceErrorWithinTolerance(double errorMm) const
{
    return std::abs(errorMm) <= m_parameters.distanceToleranceMm;
}

ArchimedeanSpiral2DGenerator::DeltaThetaResult
ArchimedeanSpiral2DGenerator::solveNextDeltaTheta(double thetaNowRad,
                                                  double thetaEndRad,
                                                  double pointSpacingMm) const
{
    switch (m_parameters.deltaThetaMethod) {
    case DeltaThetaMethod::Bisection:
        return solveNextDeltaThetaBisection(thetaNowRad, thetaEndRad, pointSpacingMm);
    }

    throw std::invalid_argument("unsupported delta theta solve method");
}

ArchimedeanSpiral2DGenerator::DeltaThetaResult
ArchimedeanSpiral2DGenerator::solveNextDeltaThetaBisection(double thetaNowRad,
                                                           double thetaEndRad,
                                                           double pointSpacingMm) const
{
    if (thetaEndRad <= thetaNowRad) {
        return {};
    }

    const double deltaThetaGuess = estimateDeltaTheta(thetaNowRad, pointSpacingMm);
    if (deltaThetaGuess <= 0.0) {
        return {};
    }

    const double thetaEstimated = thetaNowRad + deltaThetaGuess;
    if (thetaEstimated <= thetaEndRad) {
        const double estimatedError = distanceError(thetaNowRad, thetaEstimated, pointSpacingMm);
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

    double thetaLow = thetaNowRad;
    double thetaHigh = std::min(thetaEndRad, thetaNowRad + 2.0 * deltaThetaGuess);
    double highError = distanceError(thetaNowRad, thetaHigh, pointSpacingMm);

    int bracketExpansions = 0;
    while (highError < 0.0 && thetaHigh < thetaEndRad) {
        if (bracketExpansions >= m_parameters.maxBracketExpansions) {
            return {};
        }

        thetaHigh = std::min(thetaEndRad, thetaHigh + deltaThetaGuess);
        highError = distanceError(thetaNowRad, thetaHigh, pointSpacingMm);
        ++bracketExpansions;
    }

    if (highError < 0.0) {
        return {};
    }

    DeltaThetaResult result;
    result.hasSolution = true;

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
            thetaLow = thetaMid;
        } else {
            thetaHigh = thetaMid;
        }
    }

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
        if (!nearlyEqual(ranges[index].startRadiusMm,
                         expectedStart,
                         m_parameters.distanceToleranceMm)) {
            throw std::invalid_argument("radius ranges must be contiguous and sorted");
        }
    }
}

void ArchimedeanSpiral2DGenerator::appendRadiusRange(const RadiusRange& range,
                                                     std::size_t rangeIndex)
{
    const double thetaStart = thetaAtRadius(range.startRadiusMm);
    const double thetaEnd = thetaAtRadius(range.endRadiusMm);

    double thetaNow = thetaStart;
    const Point startPoint = pointAt(thetaNow, rangeIndex);
    if (m_points.empty()
        || pointDistance(m_points.back(), startPoint) > kTinyDistanceMm) {
        m_points.push_back(startPoint);
    }

    while (thetaNow < thetaEnd) {
        const DeltaThetaResult solveResult =
            solveNextDeltaTheta(thetaNow, thetaEnd, range.pointSpacingMm);

        if (!solveResult.hasSolution) {
            break;
        }

        thetaNow = solveResult.nextThetaRad;
        Point nextPoint = pointAt(thetaNow, rangeIndex);
        nextPoint.iterationsFromPreviousPoint = solveResult.iterations;
        m_points.push_back(nextPoint);

        if (thetaEnd - thetaNow <= std::numeric_limits<double>::epsilon()) {
            break;
        }
    }

    if (m_parameters.appendRangeEndPoint) {
        const Point endPoint = pointAt(thetaEnd, rangeIndex);
        if (m_points.empty()
            || pointDistance(m_points.back(), endPoint) > kTinyDistanceMm) {
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

std::vector<double>
ArchimedeanSpiral2DGenerator::computeDistances(const std::vector<Point>& points)
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
