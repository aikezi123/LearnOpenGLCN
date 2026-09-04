#include <diagnostics/ModuleLogger.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace engineeringlab::application::diagnostics {
namespace {

struct CapturedRecord {
    LogLevel level{LogLevel::Info};
    std::string component;
    std::string message;
    SourceLocation source;
};

class CapturingLogger final : public ILogger {
public:
    void write(const LogRecord& record) noexcept override
    {
        try {
            m_records.push_back(CapturedRecord{
                record.level,
                std::string(record.component),
                std::string(record.message),
                record.source
            });
        }
        catch (...) {
        }
    }

    void flush() noexcept override
    {
    }

    const std::vector<CapturedRecord>& records() const noexcept
    {
        return m_records;
    }

private:
    std::vector<CapturedRecord> m_records;
};

TEST(ModuleLoggerTest, FormatsMessageAndBindsComponent)
{
    CapturingLogger sink;
    ModuleLogger logger(sink, "camera");

    logger.info("这是日志 {}，帧率={} fps", std::string("括号里的内容"), 30);

    ASSERT_EQ(sink.records().size(), 1U);
    EXPECT_EQ(sink.records()[0].level, LogLevel::Info);
    EXPECT_EQ(sink.records()[0].component, "camera");
    EXPECT_EQ(sink.records()[0].message, "这是日志 括号里的内容，帧率=30 fps");
    EXPECT_EQ(sink.records()[0].source.file, nullptr);
}

TEST(ModuleLoggerTest, ProvidesMethodForEveryLogLevel)
{
    CapturingLogger sink;
    ModuleLogger logger(sink, "render");

    logger.trace("trace");
    logger.debug("debug");
    logger.info("info");
    logger.warning("warning");
    logger.error("error");
    logger.critical("critical");

    ASSERT_EQ(sink.records().size(), 6U);
    EXPECT_EQ(sink.records()[0].level, LogLevel::Trace);
    EXPECT_EQ(sink.records()[1].level, LogLevel::Debug);
    EXPECT_EQ(sink.records()[2].level, LogLevel::Info);
    EXPECT_EQ(sink.records()[3].level, LogLevel::Warning);
    EXPECT_EQ(sink.records()[4].level, LogLevel::Error);
    EXPECT_EQ(sink.records()[5].level, LogLevel::Critical);
}

TEST(ModuleLoggerTest, RejectsEmptyComponent)
{
    CapturingLogger sink;
    EXPECT_THROW(ModuleLogger logger(sink, ""), std::invalid_argument);
}

} // namespace
} // namespace engineeringlab::application::diagnostics
