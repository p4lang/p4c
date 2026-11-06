// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include <unistd.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#include "frontends/common/parseInput.h"
#include "frontends/common/preprocessorDiagnostics.h"
#include "ir/json_parser.h"
#include "lib/diagnostic.h"
#include "lib/error_reporter.h"
#include "lib/sarif.h"
#include "test/gtest/helpers.h"

namespace P4::Test {
namespace {

struct Located {
    Util::SourceInfo source;
    Util::SourceInfo getSourceInfo() const { return source; }
    cstring toString() const { return "value"_cs; }
};

TEST(StructuredDiagnostics, CollectionIsOptionalAndDoesNotChangeTerminalOutput) {
    std::ostringstream plain, structured;
    ErrorReporter first, second;
    first.setOutputStream(&plain);
    second.setOutputStream(&structured);
    second.enableDiagnosticCollection();
    for (auto *reporter : {&first, &second}) {
        reporter->diagnose(DiagnosticAction::Info, "info", "information %d", "", 1);
        reporter->diagnose(DiagnosticAction::Warn, "warning", "warning %d", "", 2);
        reporter->diagnose(DiagnosticAction::Error, "invalid", "error %d", "", 3);
    }
    EXPECT_EQ(plain.str(), structured.str());
    EXPECT_TRUE(first.getDiagnostics().empty());
    auto diagnostics = second.getDiagnostics();
    ASSERT_EQ(diagnostics.size(), 3u);
    EXPECT_EQ(diagnostics[0].severity, DiagnosticSeverity::Note);
    EXPECT_EQ(diagnostics[1].severity, DiagnosticSeverity::Warning);
    EXPECT_EQ(diagnostics[2].severity, DiagnosticSeverity::Error);
    EXPECT_EQ(diagnostics[2].code, "invalid");
    EXPECT_EQ(diagnostics[2].message, "error 3");
    EXPECT_FALSE(diagnostics[2].location);
}

TEST(StructuredDiagnostics, OwnedRangesRelatedLocationsAndLspMapping) {
    std::vector<Diagnostic> retained;
    std::vector<Diagnostic> observed;
    {
        std::ostringstream terminal;
        ErrorReporter reporter;
        reporter.setOutputStream(&terminal);
        reporter.setDiagnosticConsumer([&](const Diagnostic &d) { observed.push_back(d); });
        Util::InputSources sources;
        sources.mapLine("a space\"%é.p4", 7);
        sources.appendText("\n");
        sources.appendText("a😀éX");
        auto primary = Util::SourceInfo(&sources, {2, 7}, {2, 8});
        auto related = Util::SourceInfo(&sources, {2, 0}, {2, 1});
        reporter.diagnose(DiagnosticAction::Error, "invalid", "%svalue conflicts with %s", "",
                          primary, related);
        retained = reporter.getDiagnostics();
    }
    ASSERT_EQ(retained.size(), 1u);
    ASSERT_EQ(observed.size(), 1u);
    const auto &diagnostic = retained.front();
    ASSERT_TRUE(diagnostic.location);
    EXPECT_EQ(diagnostic.location->file, "a space\"%é.p4");
    EXPECT_NE(diagnostic.location->uri.find("a%20space%22%25%C3%A9.p4"), std::string::npos);
    ASSERT_TRUE(diagnostic.location->range);
    const auto &range = *diagnostic.location->range;
    EXPECT_EQ(range.start.line, 6u);
    EXPECT_EQ(range.start.column, 4u);
    ASSERT_TRUE(range.end);
    EXPECT_EQ(range.end->column, 5u);
    ASSERT_EQ(diagnostic.related.size(), 1u);
    EXPECT_EQ(diagnostic.related[0].location.range->start.column, 0u);
    EXPECT_EQ(observed[0].message, diagnostic.message);

    // A future LSP adapter selecting UTF-16 can use these zero-based positions.
    // SARIF separately adds one. Unknown columns must be handled explicitly.
    struct LspPosition {
        unsigned line;
        unsigned character;
    };
    ASSERT_TRUE(range.start.column);
    LspPosition lsp{range.start.line, *range.start.column};
    EXPECT_EQ(lsp.line, 6u);
    EXPECT_EQ(lsp.character, 4u);
}

TEST(StructuredDiagnostics, WarningPolicyAndDeduplication) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    reporter.setDefaultWarningDiagnosticAction(DiagnosticAction::Ignore);
    reporter.diagnose(DiagnosticAction::Warn, ErrorType::WARN_UNUSED, "%s", "", "ignored");
    EXPECT_TRUE(reporter.getDiagnostics().empty());
    reporter.setDefaultWarningDiagnosticAction(DiagnosticAction::Error);
    Util::InputSources sources;
    sources.mapLine("policy.p4", 1);
    sources.appendText("\n");
    sources.appendText("x");
    Located node{Util::SourceInfo(&sources, {2, 0}, {2, 1})};
    reporter.diagnose(DiagnosticAction::Warn, ErrorType::WARN_UNUSED, "%s", "", &node);
    reporter.diagnose(DiagnosticAction::Warn, ErrorType::WARN_UNUSED, "%s", "", &node);
    auto diagnostics = reporter.getDiagnostics();
    ASSERT_EQ(diagnostics.size(), 1u);
    EXPECT_EQ(diagnostics.front().severity, DiagnosticSeverity::Error);
    EXPECT_EQ(diagnostics.front().code, "unused");
    EXPECT_EQ(reporter.getErrorCount(), 1u);
}

TEST(StructuredDiagnostics, Utf16BoundariesAndUnknownColumns) {
    EXPECT_EQ(diagnosticFileUri("-"), "-");
    EXPECT_TRUE(diagnosticFileUri("(real).p4").starts_with("file://"));
    EXPECT_EQ(diagnosticUtf16Column("a😀é", 7), 4u);
    EXPECT_EQ(diagnosticUtf16Column("\tX", 1), 1u);
    EXPECT_FALSE(diagnosticUtf16Column("😀", 1));
    EXPECT_FALSE(diagnosticUtf16Column("abc", 4));
    EXPECT_FALSE(diagnosticUtf16Column("\xc0\x80", 2));
    EXPECT_FALSE(diagnosticUtf16Column("\xed\xa0\x80", 3));
}

TEST(StructuredDiagnostics, ReporterCopiesDoNotShareConsumersOrCollections) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    unsigned observations = 0;
    reporter.setDiagnosticConsumer([&](const Diagnostic &) { ++observations; });
    reporter.diagnose(DiagnosticAction::Warn, "first", "first", "");
    ErrorReporter copied(reporter);
    copied.diagnose(DiagnosticAction::Error, "second", "second", "");
    EXPECT_EQ(observations, 1u);
    EXPECT_EQ(reporter.getDiagnostics().size(), 1u);
    EXPECT_EQ(copied.getDiagnostics().size(), 2u);
}

class StructuredParserDiagnostics : public P4CTest {};

TEST_F(StructuredParserDiagnostics, UnsignedLineMarkersPreserveWrappedSourceMapping) {
    auto &reporter = BaseCompileContext::get().errorReporter();
    reporter.enableDiagnosticCollection();
    const auto source = "#line 4294967281\n" + std::string(16, '\n') + "true;";
    EXPECT_EQ(parseP4String("unsigned.p4", 1, source), nullptr);
    auto diagnostics = reporter.getDiagnostics();
    ASSERT_FALSE(diagnostics.empty());
    ASSERT_TRUE(diagnostics[0].location);
    ASSERT_TRUE(diagnostics[0].location->range);
    EXPECT_EQ(diagnostics[0].location->file, "unsigned.p4");
    EXPECT_EQ(diagnostics[0].location->range->start.line, 0u);
}

TEST_F(StructuredParserDiagnostics, ParserErrorsAndIndependentCompilations) {
    std::vector<Diagnostic> retained;
    {
        AutoCompileContext context(std::make_unique<GTestContext>());
        auto &reporter = BaseCompileContext::get().errorReporter();
        reporter.enableDiagnosticCollection();
        EXPECT_EQ(parseP4String("first.p4", 1, "this is not P4;"), nullptr);
        retained = reporter.getDiagnostics();
        ASSERT_FALSE(retained.empty());
        EXPECT_EQ(retained.front().source, "parser");
        EXPECT_EQ(retained.front().code, "parser");
    }
    {
        AutoCompileContext context(std::make_unique<GTestContext>());
        auto &reporter = BaseCompileContext::get().errorReporter();
        reporter.enableDiagnosticCollection();
        EXPECT_TRUE(reporter.getDiagnostics().empty());
        EXPECT_NE(parseP4String("second.p4", 1, "struct S { bit<8> x; }"), nullptr);
        EXPECT_TRUE(reporter.getDiagnostics().empty());
    }
    EXPECT_FALSE(retained.front().message.empty());
    ASSERT_TRUE(retained.front().location);
    EXPECT_EQ(retained.front().location->file, "first.p4");
}

TEST_F(StructuredParserDiagnostics, FrontendWarningsSuppressionAndPromotionWithoutSarif) {
    const auto source = P4_SOURCE(R"(
        @pragma deprecated "Avoid this function."
        extern bit<6> wrong();
        control c() {
            apply { bit<6> x = wrong(); }
        }
    )");
    for (auto action :
         {DiagnosticAction::Warn, DiagnosticAction::Ignore, DiagnosticAction::Error}) {
        AutoCompileContext context(std::make_unique<GTestContext>());
        auto &reporter = BaseCompileContext::get().errorReporter();
        reporter.enableDiagnosticCollection();
        reporter.setDiagnosticAction("deprecated", action);
        auto program = FrontendTestCase::create(source);
        EXPECT_EQ(program.has_value(), action != DiagnosticAction::Error);
        unsigned count = 0;
        for (const auto &record : reporter.getDiagnostics()) {
            if (record.code != "deprecated") continue;
            ++count;
            EXPECT_EQ(record.severity, action == DiagnosticAction::Error
                                           ? DiagnosticSeverity::Error
                                           : DiagnosticSeverity::Warning);
            EXPECT_NE(record.message.find("Avoid this function"), std::string::npos);
            EXPECT_TRUE(record.location);
            EXPECT_FALSE(record.related.empty());
        }
        EXPECT_EQ(count, action == DiagnosticAction::Ignore ? 0u : 1u);
    }
}

TEST(StructuredDiagnostics, SarifUsesTheSameOwnedRecordsDeterministically) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    reporter.parser_error(Util::SourceInfo{}, "bad syntax");
    std::ostringstream first, second;
    writeSarif(first, reporter.getDiagnostics());
    writeSarif(second, reporter.getDiagnostics());
    EXPECT_EQ(first.str(), second.str());
    std::istringstream input(first.str());
    std::unique_ptr<JsonData> report;
    parseJson(input, report, true);
    ASSERT_NE(report, nullptr);
    EXPECT_NE(first.str().find("\"ruleId\":\"parser\""), std::string::npos);
    EXPECT_NE(first.str().find("\"text\":\"bad syntax\""), std::string::npos);
    EXPECT_EQ(first.str().find("\"startLine\""), std::string::npos);
}

TEST(StructuredDiagnostics, InvalidPreprocessorRecordsAreVisibleAndEarlierWarningsSurvive) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    EXPECT_FALSE(reportPreprocessorDiagnostics(
        R"([{"kind":"warning","message":"earlier","locations":[]}, {"kind":"mystery"}])",
        reporter));
    auto records = reporter.getDiagnostics();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].message, "earlier");
    EXPECT_EQ(records[1].severity, DiagnosticSeverity::Error);
    EXPECT_NE(terminal.str().find("Cannot decode GCC"), std::string::npos);
    for (const auto *invalid : {"[] trailing", "[", "[{}]", "[{},]", "{}", "", "[1 2]",
                                "[{\"kind\":\"note\",\"message\":\"bad\\v\",\"locations\":[]}]"})
        EXPECT_FALSE(reportPreprocessorDiagnostics(invalid, reporter)) << invalid;
}

TEST(StructuredDiagnostics, StrictJsonUnicodeAndLegacyModeIsolation) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    auto previous = JsonData::strict;
    EXPECT_TRUE(reportPreprocessorDiagnostics(
        R"([{"kind":"note","message":"\uD83D\uDE00\/\u00E9","locations":[]}])", reporter));
    ASSERT_EQ(reporter.getDiagnostics().size(), 1u);
    EXPECT_EQ(reporter.getDiagnostics()[0].message, "😀/é");
    EXPECT_EQ(JsonData::strict, previous);
}

TEST(StructuredDiagnostics, GroupedGccErrorsRetainEarlierWarningsAndAttachNotes) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    EXPECT_TRUE(reportPreprocessorDiagnostics(
        R"([{"kind":"warning","message":"earlier","locations":[],"children":[
            {"kind":"fatal error","message":"missing","locations":[],"children":[
                {"kind":"note","message":"included here","locations":[]}]}]}])",
        reporter));
    auto records = reporter.getDiagnostics();
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].severity, DiagnosticSeverity::Warning);
    EXPECT_EQ(records[1].severity, DiagnosticSeverity::Error);
    ASSERT_EQ(records[1].related.size(), 1u);
    EXPECT_EQ(records[1].related[0].message, "included here");
    EXPECT_FALSE(reportPreprocessorDiagnostics(
        R"([{"kind":"warning","message":"valid parent","locations":[],"children":[
            {"kind":"unknown"}]}])",
        reporter));
    records = reporter.getDiagnostics();
    ASSERT_EQ(records.size(), 4u);
    EXPECT_EQ(records[2].message, "valid parent");
    EXPECT_EQ(records[3].code, "preprocessor-diagnostics");
}

TEST(StructuredDiagnostics, GccEmptyLineHasAKnownZeroColumn) {
    char path[] = "/tmp/p4c-diagnostic-source-XXXXXX";
    int descriptor = mkstemp(path);
    ASSERT_GE(descriptor, 0);
    ASSERT_EQ(write(descriptor, "\n", 1), 1);
    close(descriptor);
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    std::string json = R"([{"kind":"note","message":"blank","locations":[{"caret":{"file":")" +
                       std::string(path) + R"(","line":1,"byte-column":1}}]}])";
    ASSERT_TRUE(reportPreprocessorDiagnostics(json, reporter));
    auto records = reporter.getDiagnostics();
    ASSERT_EQ(records.size(), 1u);
    ASSERT_TRUE(records[0].location->range);
    EXPECT_EQ(records[0].location->range->start.column, 0u);
    EXPECT_EQ(std::remove(path), 0);
}

TEST(StructuredDiagnostics, DeserializedSourceKeepsIdentityWithoutInventingAColumn) {
    ErrorMessage message(ErrorMessage::MessageType::Error, "invalid", "message",
                         {Util::SourceInfo("from-json.p4"_cs, 8, 20, "fragment"_cs)}, "");
    auto diagnostic = makeDiagnostic(message);
    ASSERT_TRUE(diagnostic.location);
    EXPECT_EQ(diagnostic.location->file, "from-json.p4");
    ASSERT_TRUE(diagnostic.location->range);
    EXPECT_EQ(diagnostic.location->range->start.line, 7u);
    EXPECT_FALSE(diagnostic.location->range->start.column);
}

TEST(StructuredDiagnostics, UnavailableSourceTextDoesNotInventColumnsOrThrow) {
    Util::InputSources sources;
    sources.mapLine("buffer.p4", 2);
    ErrorMessage message(ErrorMessage::MessageType::Error, "invalid", "synthetic range",
                         {Util::SourceInfo(&sources, {1, 9}, {3, 1})}, "");
    auto diagnostic = makeDiagnostic(message);
    ASSERT_TRUE(diagnostic.location);
    ASSERT_TRUE(diagnostic.location->range);
    EXPECT_FALSE(diagnostic.location->range->start.column);
    EXPECT_FALSE(diagnostic.location->range->end);
}

TEST(StructuredDiagnostics, WriteFailureAfterInitializationIsCountedAndObservedInOrder) {
    char path[] = "/tmp/p4c-diagnostic-test-XXXXXX";
    int descriptor = mkstemp(path);
    ASSERT_GE(descriptor, 0);
    close(descriptor);
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    std::vector<std::string> observed;
    reporter.setDiagnosticConsumer([&](const Diagnostic &d) { observed.push_back(d.code); });
    ASSERT_TRUE(reporter.setDiagnosticsFile(path));
    // Replace this test's own temporary file with an unwritable directory.
    ASSERT_EQ(std::remove(path), 0);
    ASSERT_TRUE(std::filesystem::create_directory(path));
    reporter.diagnose(DiagnosticAction::Warn, "example", "warning", "");
    EXPECT_EQ(reporter.getErrorCount(), 1u);
    EXPECT_EQ(observed, (std::vector<std::string>{"example", "io"}));
    EXPECT_NE(terminal.str().find("Cannot write diagnostics file"), std::string::npos);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST(StructuredDiagnostics, SeparateLargeSubprocessOutputsDoNotDeadlock) {
    std::ostringstream terminal;
    ErrorReporter reporter;
    reporter.setOutputStream(&terminal);
    reporter.enableDiagnosticCollection();
    auto output = preprocessWithDiagnostics(
        "python3 -c 'import sys; sys.stdout.write(\"x\"*262144); "
        "sys.stderr.write(\"[{\\\"kind\\\":\\\"warning\\\",\\\"message\\\":\\\"\" + "
        "\"y\"*262144 + \"\\\",\\\"locations\\\":[]}]\")'",
        reporter);
    ASSERT_TRUE(output);
    EXPECT_EQ(std::fgetc(output.get()), 'x');
    ASSERT_EQ(reporter.getDiagnostics().size(), 1u);
    EXPECT_EQ(reporter.getDiagnostics()[0].message.size(), 262144u);
}

}  // namespace
}  // namespace P4::Test
