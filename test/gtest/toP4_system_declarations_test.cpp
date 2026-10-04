// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "frontends/common/parser_options.h"
#include "frontends/p4/toP4/toP4.h"
#include "frontends/parsers/parserDriver.h"
#include "helpers.h"

namespace P4::Test {
namespace {

class ToP4SystemDeclarationsTest : public P4CTest {
 protected:
    const IR::P4Program *parse(bool extendNamespaces = false) {
        std::stringstream source;
        source << "error { SystemError }\n"
               << "extern void library_function();\n"
               << "match_kind { system_match }\n"
               << "#line 1 \"user.p4\"\n";
        if (extendNamespaces) {
            source << "error { UserError }\nmatch_kind { user_match }\n";
        }
        source << "const bit<8> value = 1;\n";
        return P4ParserDriver::parse(source, (p4includePath / "core.p4").generic_string());
    }
};

TEST_F(ToP4SystemDeclarationsTest, SkippedDeclarationsDoNotAddSeparators) {
    const auto *program = parse();
    ASSERT_NE(program, nullptr);
    Util::SourceCodeBuilder builder;
    ToP4 printer(builder, false);
    program->apply(printer);
    EXPECT_EQ(builder.toString(), "#include <core.p4>\n\nconst bit<8> value = 1;\n");
}

TEST_F(ToP4SystemDeclarationsTest, RetainsUserExtensionsToSystemNamespaces) {
    const auto *program = parse(true);
    ASSERT_NE(program, nullptr);
    Util::SourceCodeBuilder builder;
    ToP4 printer(builder, false);
    program->apply(printer);
    const auto output = builder.toString();
    EXPECT_NE(output.find("error {\n    UserError\n}"), std::string::npos);
    EXPECT_NE(output.find("match_kind {\n    user_match\n}"), std::string::npos);
    EXPECT_NE(output.find("#include <core.p4>"), std::string::npos);
    EXPECT_EQ(output.find("SystemError"), std::string::npos);
    EXPECT_EQ(output.find("system_match"), std::string::npos);
}

TEST_F(ToP4SystemDeclarationsTest, RetainsSystemDeclarationsWhenIncludesAreDisabled) {
    const auto *program = parse();
    ASSERT_NE(program, nullptr);
    Util::SourceCodeBuilder builder;
    ToP4 printer(builder, false);
    printer.setnoIncludesArg(true);
    program->apply(printer);
    const auto output = builder.toString();
    EXPECT_NE(output.find("error {\n    SystemError\n}"), std::string::npos);
    EXPECT_NE(output.find("match_kind {\n    system_match\n}"), std::string::npos);
    EXPECT_NE(output.find("extern void library_function();"), std::string::npos);
    EXPECT_EQ(output.find("#include"), std::string::npos);
}

}  // namespace
}  // namespace P4::Test
