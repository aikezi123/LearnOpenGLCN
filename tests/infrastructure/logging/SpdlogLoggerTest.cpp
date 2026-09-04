#include <logging/SpdlogLogger.h>

#include <diagnostics/ModuleLogger.h>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace engineeringlab::infrastructure::logging {
namespace {

class TemporaryLogDirectory final {
public:
    TemporaryLogDirectory()
    {
        const auto uniqueSuffix = std::chrono::steady_clock::now()
                                      .time_since_epoch()
                                      .count();
        m_path = std::filesystem::temp_directory_path()
            / ("EngineeringLabLoggingTest-" + std::to_string(uniqueSuffix));
        std::filesystem::create_directories(m_path);
    }

    ~TemporaryLogDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    const std::filesystem::path& path() const noexcept
    {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()
    );
}

application::diagnostics::LogRecord makeRecord(
    application::diagnostics::LogLevel level,
    std::string_view message
)
{
    return {
        level,
        "logging-test",
        message,
        {__FILE__, __LINE__, __func__}
    };
}

TEST(SpdlogLoggerTest, WritesUtf8RecordAndFiltersByMinimumLevel)
{
    TemporaryLogDirectory directory;
    const std::filesystem::path logFile = directory.path() / "sync.log";

    SpdlogLoggerOptions options;
    options.logFile = logFile;
    options.minimumLevel = application::diagnostics::LogLevel::Warning;
    options.asynchronous = false;

    {
        SpdlogLogger logger(options);
        logger.write(makeRecord(
            application::diagnostics::LogLevel::Info,
            "filtered-info"
        ));
        logger.write(makeRecord(
            application::diagnostics::LogLevel::Warning,
            "相机打开失败"
        ));
        logger.flush();
    }

    const std::string contents = readFile(logFile);
    EXPECT_EQ(contents.find("filtered-info"), std::string::npos);
    EXPECT_NE(contents.find("[logging-test] 相机打开失败"), std::string::npos);
    EXPECT_NE(contents.find("[warning]"), std::string::npos);
    EXPECT_NE(contents.find("(SpdlogLoggerTest.cpp:"), std::string::npos);
}

TEST(SpdlogLoggerTest, AsyncLoggerDrainsQueueDuringDestruction)
{
    TemporaryLogDirectory directory;
    const std::filesystem::path logFile = directory.path() / "async.log";

    SpdlogLoggerOptions options;
    options.logFile = logFile;
    options.asynchronous = true;
    options.asyncQueueCapacity = 16U;

    {
        SpdlogLogger logger(options);
        logger.write(makeRecord(
            application::diagnostics::LogLevel::Info,
            "async-message"
        ));
    }

    const std::string contents = readFile(logFile);
    EXPECT_NE(contents.find("async-message"), std::string::npos);
}

TEST(SpdlogLoggerTest, RejectsEmptyLogFilePath)
{
    SpdlogLoggerOptions options;
    EXPECT_THROW(SpdlogLogger logger(options), std::invalid_argument);
}

TEST(SpdlogLoggerTest, ModuleLoggerWritesFormattedMessageWithoutEmptySourceMarker)
{
    TemporaryLogDirectory directory;
    const std::filesystem::path logFile = directory.path() / "module.log";

    SpdlogLoggerOptions options;
    options.logFile = logFile;
    options.asynchronous = false;

    SpdlogLogger backend(options);
    application::diagnostics::ModuleLogger logger(backend, "camera");
    logger.info("Camera {} opened at {} fps.", 7, 30);
    backend.flush();

    const std::string contents = readFile(logFile);
    EXPECT_NE(contents.find("[camera] Camera 7 opened at 30 fps."), std::string::npos);
    EXPECT_EQ(contents.find("(:)"), std::string::npos);
}

} // namespace
} // namespace engineeringlab::infrastructure::logging
