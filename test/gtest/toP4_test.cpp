// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include "frontends/p4/toP4/toP4.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "frontends/parsers/parserDriver.h"
#include "helpers.h"

namespace P4::Test {
namespace {

class ToP4Test : public P4CTest {
 protected:
    const IR::P4Program *parse() {
        std::istringstream source(R"(
            struct S { bit<8> x; bit<128> longer; }
            control C(in S h) {
                table t {
                    key = { h.x : exact; h.longer : exact; }
                    actions = {}
                }
                apply {}
            }
        )");
        return P4ParserDriver::parse(source, "top4-test.p4");
    }
};

TEST_F(ToP4Test, DefaultLayout) {
    const auto *program = parse();
    ASSERT_NE(program, nullptr);
    Util::SourceCodeBuilder builder;
    ToP4 printer(builder, false);
    program->apply(printer);
    const auto output = builder.toString();
    EXPECT_NE(output.find("    bit<8>   x;\n    bit<128> longer;"), std::string::npos);
    EXPECT_NE(output.find("            h.x     : exact;\n            h.longer: exact;"),
              std::string::npos);

    // Explicit default options and the ostream API must preserve the same output.
    std::ostringstream stream;
    ToP4 streamPrinter(&stream, false, ToP4FormattingOptions{});
    program->apply(streamPrinter);
    EXPECT_EQ(stream.str(), output);
}

TEST_F(ToP4Test, SharedIndentWidth) {
    ToP4FormattingOptions options;
    options.indentWidth = 2;
    Util::SourceCodeBuilder irBuilder;
    ToP4 irPrinter(irBuilder, false, options);
    parse()->apply(irPrinter);
    EXPECT_NE(irBuilder.toString().find("\n  bit<8>"), std::string::npos);

    Util::SourceCodeBuilder sourceBuilder;
    ToP4 sourcePrinter(sourceBuilder, false, options);
    sourcePrinter.printSource({{"control C() {", "", SourceFragment::Break::None, 0},
                               {"apply {}", "", SourceFragment::Break::Line, 1},
                               {"}", "", SourceFragment::Break::Line, 0},
                               {"", "", SourceFragment::Break::Line, 0}});
    EXPECT_EQ(sourceBuilder.toString(), "control C() {\n  apply {}\n}\n");
}

TEST_F(ToP4Test, IndependentAlignmentOptions) {
    const auto *program = parse();
    ASSERT_NE(program, nullptr);
    for (bool alignFields : {false, true}) {
        for (bool alignKeys : {false, true}) {
            SCOPED_TRACE(::testing::Message() << alignFields << ", " << alignKeys);
            ToP4FormattingOptions options;
            options.alignStructFields = alignFields;
            options.alignTableKeys = alignKeys;
            Util::SourceCodeBuilder builder;
            ToP4 printer(builder, false, std::filesystem::path("top4-test.p4"), options);
            program->apply(printer);
            const auto output = builder.toString();
            EXPECT_NE(output.find(alignFields ? "bit<8>   x;" : "bit<8> x;"), std::string::npos);
            EXPECT_NE(output.find(alignKeys ? "h.x     : exact;" : "h.x: exact;"),
                      std::string::npos);

            // Each combination must still produce parseable P4.
            std::istringstream formatted(output);
            EXPECT_NE(P4ParserDriver::parse(formatted, "formatted.p4"), nullptr);
        }
    }
}

}  // namespace
}  // namespace P4::Test
