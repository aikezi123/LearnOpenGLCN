#include <trajectory/ArchimedeanSpiral2DGenerator.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using engineeringlab::domain::trajectory::ArchimedeanSpiral2DGenerator;

namespace {

void writeTrajectoryFiles(const std::filesystem::path& xyPath,
                          const std::filesystem::path& iterationsPath,
                          const std::vector<ArchimedeanSpiral2DGenerator::Point>& points)
{
    std::ofstream xyFile(xyPath);
    std::ofstream iterationsFile(iterationsPath);
    if (!xyFile || !iterationsFile) {
        throw std::runtime_error("failed to open output file");
    }

    xyFile << std::setprecision(17);
    iterationsFile << std::setprecision(17);

    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto& point = points[index];
        xyFile << point.xMm << ' ' << point.yMm << '\n';
        iterationsFile << (index + 1) << ' ' << point.iterationsFromPreviousPoint << '\n';
    }
}

std::string rangeFilePrefix(std::size_t rangeIndex)
{
    return "archimedean_spiral_range_" + std::to_string(rangeIndex + 1);
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: export_archimedean_spiral_txt <output_directory>\n";
        return 1;
    }

    const std::filesystem::path outputDirectory = argv[1];
    std::filesystem::create_directories(outputDirectory);

    ArchimedeanSpiral2DGenerator::GenerationParameters parameters;
    parameters.startRadiusMm = 0.0;
    parameters.trackSpacingMm = 0.004;
    parameters.distanceToleranceMm = 1e-6;
    parameters.maxIterations = 30;
    parameters.maxBracketExpansions = 32;
    parameters.deltaThetaMethod = ArchimedeanSpiral2DGenerator::DeltaThetaMethod::Bisection;
    parameters.appendRangeEndPoint = true;

    const std::vector<ArchimedeanSpiral2DGenerator::RadiusRange> ranges{
        ArchimedeanSpiral2DGenerator::RadiusRange{0.0, 0.3, 0.005},
        ArchimedeanSpiral2DGenerator::RadiusRange{0.3, 1.0, 0.010},
        ArchimedeanSpiral2DGenerator::RadiusRange{1.0, 3.5, 0.050},
    };

    ArchimedeanSpiral2DGenerator generator(parameters);
    generator.generate(ranges);

    const auto& points = generator.points();

    const std::filesystem::path xyPath = outputDirectory / "archimedean_spiral_xy.txt";
    const std::filesystem::path iterationsPath = outputDirectory / "archimedean_spiral_iterations.txt";

    writeTrajectoryFiles(xyPath, iterationsPath, points);

    for (std::size_t rangeIndex = 0; rangeIndex < ranges.size(); ++rangeIndex) {
        ArchimedeanSpiral2DGenerator rangeGenerator(parameters);
        rangeGenerator.generateInRadiusRange(ranges[rangeIndex]);

        const std::string prefix = rangeFilePrefix(rangeIndex);
        writeTrajectoryFiles(outputDirectory / (prefix + "_xy.txt"),
                             outputDirectory / (prefix + "_iterations.txt"),
                             rangeGenerator.points());
    }

    const auto maxIterationPoint =
        std::max_element(points.begin(), points.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.iterationsFromPreviousPoint < rhs.iterationsFromPreviousPoint;
        });

    std::cout << "points=" << points.size() << '\n';
    std::cout << "xy=" << xyPath.string() << '\n';
    std::cout << "iterations=" << iterationsPath.string() << '\n';
    std::cout << "ranges=" << ranges.size() << '\n';
    if (maxIterationPoint != points.end()) {
        std::cout << "max_iterations=" << maxIterationPoint->iterationsFromPreviousPoint << '\n';
    }

    return 0;
}
