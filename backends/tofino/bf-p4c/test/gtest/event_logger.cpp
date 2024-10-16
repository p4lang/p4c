/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <unistd.h>
#include <fstream>
#include "bf_gtest_helpers.h"
#include "gtest/gtest.h"
#include "bf-p4c/logging/event_logger.h"
#include "lib/source_file.h"
#include "frontends/parsers/parserDriver.h"
#include "bf-p4c/bf-p4c-options.h"

namespace P4::Test {
class EventLoggerTestable : public EventLogger {
 private:
    int getTimeDifference() const override {
        return 0;
    }

    std::string getStartTimestamp() const override {
        return "TIMESTAMP";
    }

    std::ostream &getDebugStream(unsigned, const std::string &) const override {
        return std::clog;
    }

 public:
    static EventLoggerTestable &get2() {
        static EventLoggerTestable instance;
        return instance;
    }

    static DebugHook getDebugHook2() {
        return [] (const char *m, unsigned s, const char *p, const IR::Node*) {
            get2().passChange(m, p, s);
        };
    }

    void restore() {
        EventLogger::nullInit();
    }
};

class EventLoggerTest : public ::testing::Test {
 protected:
    using EventLogger = EventLoggerTestable;
    const std::string OUTDIR = "/tmp";
    const std::string FILE = "event-log-gtest.json";
    const std::string PATH = OUTDIR + "/" + FILE;

    // We need random SourceInfo initialized from InputSources object
    // That is achieved by parsing a piece of code info IR and retrieving
    // some random SourceInfo
    std::istringstream inputCode = std::istringstream("header hdr { bit<8> field; }");
    const IR::P4Program *program = P4::P4ParserDriver::parse(inputCode, "file.cpp", 1);
    const Util::SourceInfo srcInfo = program->objects[0]->srcInfo;

    const std::string SCHEMA_VERSION = R"("schema_version":"1.2.0")";
    const std::string EVENT_IDS = R"("event_ids":["Properties","Pass Changed","Parse Error","Compilation Error","Compilation Warning","Debug","Decision","Pipe Changed","Iteration Changed"])";
    const std::string DEFAULT_PROPERTIES = R"({"enabled":true,)" + EVENT_IDS + R"(,"file_ids":[],"i":0,"manager_ids":[],)" + SCHEMA_VERSION + R"(,"start_time":"TIMESTAMP"})";
    const std::string DEFAULT_PROPERTIES_DISABLED = R"({"enabled":false,)" + EVENT_IDS + R"(,"file_ids":[],"i":0,"manager_ids":[],)" + SCHEMA_VERSION + R"(,"start_time":"TIMESTAMP"})";
    const std::string PROPERTIES_WITH_FILE = R"({"enabled":true,)" + EVENT_IDS + R"(,"file_ids":["file.cpp"],"i":0,"manager_ids":[],)" + SCHEMA_VERSION + R"(,"start_time":"TIMESTAMP"})";
    const std::string PROPERTIES_WITH_MGRS = R"({"enabled":true,)" + EVENT_IDS + R"(,"file_ids":[],"i":0,"manager_ids":["mgr","mgr2"],)" + SCHEMA_VERSION + R"(,"start_time":"TIMESTAMP"})";

    void SetUp() {
        /**
         * If you execute WHOLE testing suite then EventLogger instance already exists and
         * nullInit has been called. But EventLoggerTestable does not exist and get2() will result
         * in re-executing nullInit (which is an error). Because of that, setup deinits EventLogger
         * using get(), then uses get2() to instantiate EventLoggerTestable and setup logger.
         */
        ::EventLogger::get().deinit();
        EventLogger::get2();

        // Unlink any testing artifacts that may not have been cleaned if tests segfaulted
        unlink(PATH.c_str());
    }

    void TearDown() {
        // Clear logger
        deinitLogger();

        // We need to restore logger in order for follow-up non-EventLogger regression to work
        EventLogger::get2().restore();

        // Clean
        unlink(PATH.c_str());
    }

    void initLogger(bool enable = true) {
        EventLogger::get2().init(OUTDIR, FILE);
        if (enable) EventLogger::get2().enable();
    }

    void deinitLogger() {
        EventLogger::get2().deinit();
    }

    void compareFileWithExpected(std::ifstream& file,
                                 const std::vector<std::string>& expectedLines) {
        {  // unpredicated inner scope for controlling the lifetime of the temporary
            std::string dev_null;
            std::getline(file, dev_null);  // throw away the first line [the one with the metadata]
        }
        for (auto &expectedLine : expectedLines) {
            std::string actualLine;
            std::getline(file, actualLine);

            EXPECT_EQ(expectedLine, actualLine);
        }

        // Eat extra line at end to trigger EOF
        std::string nulls;
        std::getline(file, nulls);
    }
};

TEST_F(EventLoggerTest, DoesNothingWithoutInit) {
    // Expecting this logfile doesn't exist because
    // logger was not initialized
    EventLogger::get2().debug(6, "file.cpp", "Debug");

    std::ifstream load(PATH);
    EXPECT_FALSE(load.good());
}

TEST_F(EventLoggerTest, InitializedButDisabled) {
    initLogger(false);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        DEFAULT_PROPERTIES_DISABLED
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, DoesNotExportEventsWhenDisabled) {
    using Type = ErrorMessage::MessageType;
    auto debugHook = EventLogger::getDebugHook2();

    ParserErrorMessage pemsg(srcInfo, "Parser error");
    ErrorMessage cemsg(Type::Error, "", "Plain error", {}, "");
    ErrorMessage cwmsg(Type::Warning, "", "Plain warning", {}, "");

    initLogger(false);
    EventLogger::get2().parserError(pemsg);
    EventLogger::get2().error(cemsg);
    EventLogger::get2().warning(cwmsg);
    EventLogger::get2().debug(6, "file.cpp", "Debug");
    EventLogger::get2().decision(3, "file.cpp", "Description", "Picked decision", "Reason");
    debugHook("mgr", 0, "pass", nullptr);
    EventLogger::get2().pipeChange(1);
    EventLogger::get2().iterationChange(1, EventLogger::AllocPhase::PhvAllocation);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        DEFAULT_PROPERTIES_DISABLED
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsEnabledEventLogProperties) {
    initLogger();
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        DEFAULT_PROPERTIES
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsParserError) {
    ParserErrorMessage msg(srcInfo, "Parser error");

    initLogger();
    EventLogger::get2().parserError(msg);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"i":2,"m":"Parser error","si":{"c":7,"f":0,"l":0,"p":"header hdr { bit<8> field; }\n       ^^^\n"},"t":0})",
        PROPERTIES_WITH_FILE
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsEventCompilationError) {
    using Type = ErrorMessage::MessageType;

    ErrorMessage msg1(Type::Error, "", "Plain error", {}, "");
    ErrorMessage msg2(Type::Error, "", "Error with srcinfo", { srcInfo}, "");
    ErrorMessage msg3(Type::Error, "type", "Error with type", {}, "");
    ErrorMessage msg4(Type::Error, "type", "Error with type and srcinfo", { srcInfo }, "");
    ErrorMessage msg5(Type::Error, "", "Error with suffix", {}, "suffix");

    initLogger();
    EventLogger::get2().error(msg1);
    EventLogger::get2().error(msg2);
    EventLogger::get2().error(msg3);
    EventLogger::get2().error(msg4);
    EventLogger::get2().error(msg5);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"i":3,"m":"Plain error","t":0})",
        R"({"i":3,"m":"Error with srcinfo","t":0,"si":[{"c":7,"f":0,"l":0,"p":"header hdr { bit<8> field; }\n       ^^^\n"}]})",
        R"({"i":3,"m":"Error with type","t":0,"cn":"type"})",
        R"({"i":3,"m":"Error with type and srcinfo","t":0,"cn":"type","si":[{"c":7,"f":0,"l":0,"p":"header hdr { bit<8> field; }\n       ^^^\n"}]})",
        R"({"i":3,"m":"Error with suffix","t":0,"a":"suffix"})",
        PROPERTIES_WITH_FILE
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsEventCompilationWarning) {
    using Type = ErrorMessage::MessageType;

    ErrorMessage msg1(Type::Warning, "", "Plain warning", {}, "");
    ErrorMessage msg2(Type::Warning, "", "Warning with srcinfo", { srcInfo}, "");
    ErrorMessage msg3(Type::Warning, "type", "Warning with type", {}, "");
    ErrorMessage msg4(Type::Warning, "type", "Warning with type and srcinfo", { srcInfo }, "");
    ErrorMessage msg5(Type::Warning, "", "Warning with suffix", {}, "suffix");

    initLogger();
    EventLogger::get2().warning(msg1);
    EventLogger::get2().warning(msg2);
    EventLogger::get2().warning(msg3);
    EventLogger::get2().warning(msg4);
    EventLogger::get2().warning(msg5);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"i":4,"m":"Plain warning","t":0})",
        R"({"i":4,"m":"Warning with srcinfo","t":0,"si":[{"c":7,"f":0,"l":0,"p":"header hdr { bit<8> field; }\n       ^^^\n"}]})",
        R"({"i":4,"m":"Warning with type","t":0,"cn":"type"})",
        R"({"i":4,"m":"Warning with type and srcinfo","t":0,"cn":"type","si":[{"c":7,"f":0,"l":0,"p":"header hdr { bit<8> field; }\n       ^^^\n"}]})",
        R"({"i":4,"m":"Warning with suffix","t":0,"a":"suffix"})",
        PROPERTIES_WITH_FILE
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsEventDebug) {
    initLogger();
    EventLogger::get2().debug(6, "file.cpp", "Debug");
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"f":0,"i":5,"m":"Debug","t":0,"v":6})",
        PROPERTIES_WITH_FILE
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsEventDecision) {
    initLogger();
    EventLogger::get2().decision(3, "file.cpp", "Description", "Picked decision", "Reason");
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"d":"Picked decision","f":0,"i":6,"m":"Description","r":"Reason","t":0,"v":3})",
        PROPERTIES_WITH_FILE
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsDeduplicatedPassChange) {
    auto debugHook = EventLogger::getDebugHook2();

    initLogger();
    debugHook("mgr", 0, "pass", nullptr);
    debugHook("mgr", 1, "pass2", nullptr);
    debugHook("mgr", 1, "pass2", nullptr);  // this message should be deduplicated
    debugHook("mgr2", 0, "pass", nullptr);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"i":1,"mgr":0,"n":"pass","s":0,"t":0})",
        R"({"i":1,"mgr":0,"n":"pass2","s":1,"t":0})",
        R"({"i":1,"mgr":1,"n":"pass","s":0,"t":0})",
        PROPERTIES_WITH_MGRS
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsPipeChange) {
    initLogger();
    EventLogger::get2().pipeChange(1);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"i":7,"pipe":1,"t":0})",
        DEFAULT_PROPERTIES
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

TEST_F(EventLoggerTest, ExportsIterationChange) {
    initLogger();
    EventLogger::get2().iterationChange(1, EventLogger::AllocPhase::PhvAllocation);
    EventLogger::get2().iterationChange(2, EventLogger::AllocPhase::TablePlacement);
    deinitLogger();

    std::ifstream load(PATH);
    EXPECT_TRUE(load.good());

    std::vector<std::string> expectedLines = {
        R"({"i":8,"num":1,"phase":"phv_allocation","t":0})",
        R"({"i":8,"num":2,"phase":"table_placement","t":0})",
        DEFAULT_PROPERTIES
    };
    compareFileWithExpected(load, expectedLines);

    EXPECT_TRUE(load.eof());
}

}  // namespace P4::Test
