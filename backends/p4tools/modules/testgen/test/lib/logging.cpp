// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
//
// SPDX-License-Identifier: Apache-2.0

#include "backends/p4tools/common/lib/logging.h"

#include <gtest/gtest.h>

#include <functional>
#include <sstream>

#include "lib/timer.h"

namespace P4::P4Tools::Test {
namespace {

class LoggingTest : public testing::Test {
 protected:
    std::ostringstream output;
    std::streambuf *previous = nullptr;

    void SetUp() override {
        Log::addDebugSpec("p4tools_logging_test:4");
        previous = std::clog.rdbuf(output.rdbuf());
    }
    void TearDown() override { std::clog.rdbuf(previous); }
};

struct StreamValue {
    StreamValue() = default;
    StreamValue(const StreamValue &) = delete;
    std::string toString() const { return "toString"; }
    void dbprint(std::ostream &out) const { out << "dbprint"; }
    friend std::ostream &operator<<(std::ostream &out, const StreamValue &) {
        return out << "stream";
    }
    friend std::ostream &operator<<(std::ostream &out, const StreamValue *) {
        return out << "pointer";
    }
};

TEST_F(LoggingTest, LegacyFormatsAndStreamInsertion) {
    StreamValue value;
    printFeature("p4tools_logging_test", 4, "%2% %1% %3% %4% %%", value, 7, &value,
                 std::cref(value));
    EXPECT_TRUE(output.str().ends_with("7 stream pointer stream %\n")) << output.str();
}

TEST_F(LoggingTest, PerformanceReport) {
    Util::withTimer("p4tools_logging_timer", [] {});
    enablePerformanceLogging();
    EXPECT_NO_THROW(printPerformanceReport(std::nullopt));
    Log::addDebugSpec("tools_performance:0");
    EXPECT_NE(output.str().find("p4tools_logging_timer: "), std::string::npos) << output.str();
    EXPECT_NE(output.str().find("ms per invocation, "), std::string::npos) << output.str();
    EXPECT_NE(output.str().find("% of parent)"), std::string::npos) << output.str();
}

TEST_F(LoggingTest, DisabledLoggingDoesNotFormat) {
    Log::addDebugSpec("p4tools_logging_disabled:0");
    EXPECT_NO_THROW(printFeature("p4tools_logging_disabled", 4, "%q", StreamValue{}));
    EXPECT_TRUE(output.str().empty());
}

}  // namespace
}  // namespace P4::P4Tools::Test
