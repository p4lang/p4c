/*
Copyright 2024 Intel Corp.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <gtest/gtest.h>

#include <regex>
#include <string>

#include "frontends/common/parseInput.h"
#include "helpers.h"
#include "midend/def_use.h"
#include "midend_pass.h"

using namespace P4;

namespace Test {

using P4TestContext = P4CContextWithOptions<CompilerOptions>;

class P4CMidendDefUse : public P4CTest {};

/// Run the midend and return the ComputeDefUse object
P4::ComputeDefUse *computeDefUse(std::string source, CompilerOptions::FrontendVersion langVersion =
                                                         CompilerOptions::FrontendVersion::P4_16) {
    AutoCompileContext autoP4TestContext(new P4TestContext);

    auto *program = P4::parseP4String(source, langVersion);
    CHECK_NULL(program);
    BUG_CHECK(::errorCount() == 0, "Unexpected errors");

    auto &options = P4TestContext::get().options();
    const char *argv = "./gtestp4c";
    options.process(1, (char *const *)&argv);
    options.langVersion = CompilerOptions::FrontendVersion::P4_16;

    P4::FrontEnd frontend;
    program = frontend.run(options, program);
    CHECK_NULL(program);

    MidEnd midEnd(options);
    const IR::P4Program *res = program;
    midEnd.process(res);

    return midEnd.defuse;
}

/// Get two vectors of strings: one corresponding to the defs and one corresponding to the uses of
/// the defuse oject.
std::pair<std::vector<std::string>, std::vector<std::string>> get_defs_uses(
    const P4::ComputeDefUse *defuse, bool dump = false) {
    std::vector<std::string> defs;
    std::vector<std::string> uses;

    std::stringstream defuse_ss;
    defuse_ss << *defuse;

    std::vector<std::string> *target = &defs;
    std::string line;
    while (std::getline(defuse_ss, line)) {
        if (line == "defs:") {
            target = &defs;
        } else if (line == "uses:") {
            target = &uses;
        } else {
            target->push_back(line);
        }
    }

    // Debug output
    if (dump) {
        std::cerr << "defs:" << std::endl;
        for (auto &line : defs) std::cerr << line << std::endl;
        std::cerr << "uses:" << std::endl;
        for (auto &line : uses) std::cerr << line << std::endl;
    }

    return std::make_pair(defs, uses);
}

/// Check whether a corresponding def-use line exists in the defuse output
///
/// Def/use lines are of the form:
///     h.h1.f1[31:31]<200396>(control:3:21): { <129206>(control:0:47) }
///
/// @param lines      The string dump from the defuse object.
/// @param lhs        The "left-hand side" term to search for (`h.h1.f1[31:31]` in the example).
/// @param lhs_line   The line number for the @p lhs parameter (3 in the example).
/// @param rhs_lines  A set of line numbers to expect on the "right-hand side" of the expression
/// ({ 0 } in the example).
///
/// @return boolean indicating if any of the def-use lines match.
bool check_def_use(const std::vector<std::string> &lines, std::string lhs, unsigned lhs_line,
                   std::set<unsigned> rhs_lines) {
    // Regex to apply to the whole line
    const std::regex line_regex(R"((\w[^<]*)<\d+>\([^)]+:(\d+):[^)]+\): \{ (.*) \})");

    // Regex to apply to elements on the "right-hand side" -- the elements within the braces in the
    // line
    const std::regex element_regex(R"(<\d+>\([^)]+:(\d+):[^)]+\))");

    for (auto &line : lines) {
        std::smatch match;
        if (std::regex_search(line, match, line_regex)) {
            // Check if the line corresponds to the lhs + lhs_line
            if (match[1] == lhs && std::stoul(match[2]) == lhs_line) {
                // auto target_lines = rhs_lines;

                // Split the right-hand side into the component elements
                size_t pos = 0;
                std::string rhs = match[3];
                std::vector<std::string> elements;
                while ((pos = rhs.find(", ")) != std::string::npos) {
                    elements.push_back(rhs.substr(0, pos));
                    rhs.erase(0, pos + 2);
                }
                elements.push_back(rhs);

                // Verify that the elements match the rhs_lines
                std::set<unsigned> target_lines;
                for (auto &element : elements) {
                    if (std::regex_match(element, match, element_regex)) {
                        // if (!target_lines.erase(std::stoul(match[1]))) miss = true;
                        target_lines.emplace(std::stoul(match[1]));
                    }
                }
                if (target_lines == rhs_lines) return true;
            }
        }
    }

    return false;
}

/// Trim any leading whitespace from a string
/// @returns Trimmed version of the string
std::string ltrim(const std::string &str) {
    const std::string whitespace = " \n\r\t";

    auto pos = str.find_first_not_of(whitespace);
    return (pos == std::string::npos) ? "" : str.substr(pos);
}

/// Construct a program string, substituting in sections for the header definitions, parser body,
/// main control body, and deparser body.
///
/// The body parameters should exclude the section declaration and opening/closing braces. For
/// example, the parser body should consist of only the state definitions (and any prededing
/// declarations) -- it should not include `parser ParserName(parameters)`.
///
/// The resulting string contains pre-processor declarations the name each of the sections:
/// "header", "parser", "control", and "deparser". This is helpful when checking the def-use
/// results: line numbers are relative to the section beginning. The `parser`/`control` declarations
/// (including parameters) are line 0; the body always starts at line 1.
///
/// Note: leading whitespace is stripped from the headers and body definitions.
std::string make_program(const std::string &headers, const std::string &parser_body,
                         const std::string &control_body, const std::string &deparser_body) {
    std::string program = R"(
        // simplified core.p4
        error {
            NoError,           /// No error.
            PacketTooShort,    /// Not enough bits in packet for 'extract'.
            NoMatch,           /// 'select' expression has no matches.
            StackOutOfBounds,  /// Reference to invalid element of a header stack.
            HeaderTooShort,    /// Extracting too many bits into a varbit field.
            ParserTimeout,     /// Parser execution time limit exceeded.
            ParserInvalidArgument  /// Parser operation was called with a value
                                   /// not supported by the implementation.
        }
        extern void verify(in bool check, in error toSignal);
        extern packet_in {
            void extract<T> (out T hdr);
        }
        extern packet_out {
            void emit<T> (in T hdr);
        }

        // simplified v1model.p4
        parser Parser<H, M> (packet_in b, 
                          out H parsedHdr,
                          out M meta);
        control Ingress<H, M> (inout H hdr, inout M meta);
        control Deparser<H> (packet_out b, in H hdr);
        package PSA<H, M> (Parser<H, M> p, Ingress<H, M> ig, Deparser<H> dp);

        // user program
# 1 "headers" 1
        )" + ltrim(headers) +
                          R"(
# 0 "parser" 1
        parser MyParser(packet_in pkt, out ParsedHeaders h, out Metadata m) {
        )" + ltrim(parser_body) +
                          R"(
        }
# 0 "control" 1
        control MyIngress(inout ParsedHeaders h, inout Metadata m) {
        )" + ltrim(control_body) +
                          R"(
        }
# 0 "deparser" 1
        control MyDeparser(packet_out b, in ParsedHeaders h) {
        )" + ltrim(deparser_body) +
                          R"(
        }
        PSA(MyParser(), MyIngress(), MyDeparser()) main;
    )";

    return P4_SOURCE(program.c_str());
}

TEST_F(P4CMidendDefUse, whole_field_1) {
    std::string headers = R"(
        header hdr_h_t {
            bit<16> f1;
            bit<16> f2;
        }
        header hdr_b_t {
            bit<8> f1;
            bit<8> f2;
        }
        struct ParsedHeaders {
            hdr_h_t h1;
            hdr_b_t h2;
        }
        struct Metadata {
            bit<32> hdr;
        }
    )";
    std::string parser_body = R"(
        state start {
            pkt.extract(h.h1);
            m.hdr = 0;
            transition accept;
        }
    )";
    std::string control_body = R"(
        apply {
            h.h1.f2 = 16w0x1234;

            if (h.h1.f1 == h.h1.f2) {
                h.h1.f1 = 0;
            }
        }
    )";
    std::string deparser_body = R"(
        apply {
            b.emit(h);
        }
    )";

    auto program = make_program(headers, parser_body, control_body, deparser_body);
    auto *defuse = computeDefUse(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(defuse);

    auto [defs, uses] = get_defs_uses(defuse);

    EXPECT_TRUE(check_def_use(defs, "h.h1.f1", 4, {0}));
    // Note: no def for h.h1.f2 as the use on line 4 is replaced by a constant
    EXPECT_TRUE(check_def_use(defs, "inout ParsedHeaders h", 0, {0, 2, 5}));

    EXPECT_TRUE(check_def_use(uses, "h.h1.f1", 5, {0}));
    EXPECT_TRUE(check_def_use(uses, "h.h1.f2", 2, {0}));
    EXPECT_TRUE(check_def_use(uses, "inout ParsedHeaders h", 0, {0, 4}));
}

TEST_F(P4CMidendDefUse, whole_field_2) {
    std::string headers = R"(
        header hdr_h_t {
            bit<16> f1;
            bit<16> f2;
        }
        header hdr_b_t {
            bit<8> f1;
            bit<8> f2;
        }
        struct ParsedHeaders {
            hdr_h_t h1;
            hdr_b_t h2;
        }
        struct Metadata {
            bit<32> hdr;
        }
    )";
    std::string parser_body = R"(
        state start {
            pkt.extract(h.h1);
            m.hdr = 0;
            transition accept;
        }
    )";
    std::string control_body = R"(
        apply {
            if (h.h1.f1 == 0x1234) {
                h.h2.setValid();
                h.h2.f1 = 0x01;
                h.h2.f2 = 0x23;
            }
        }
    )";
    std::string deparser_body = R"(
        apply {
            b.emit(h);
        }
    )";

    auto program = make_program(headers, parser_body, control_body, deparser_body);
    auto *defuse = computeDefUse(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(defuse);

    auto [defs, uses] = get_defs_uses(defuse);

    EXPECT_TRUE(check_def_use(defs, "h.h1.f1", 2, {0}));
    EXPECT_TRUE(check_def_use(defs, "inout ParsedHeaders h", 0, {0, 4, 5}));

    EXPECT_TRUE(check_def_use(uses, "h.h2.f1", 4, {0}));
    EXPECT_TRUE(check_def_use(uses, "h.h2.f2", 5, {0}));
    EXPECT_TRUE(check_def_use(uses, "inout ParsedHeaders h", 0, {0, 2}));
}

TEST_F(P4CMidendDefUse, slice_1) {
    std::string headers = R"(
        header hw_t {
            bit<32> f1;
        }
        header hb_t {
            bit<8> f1;
        }
        struct ParsedHeaders {
            hw_t h1;
            hb_t h2;
            hb_t h3;
        }
        struct Metadata {
            bit<32> hdr;
        }
    )";
    std::string parser_body = R"(
        state start {
            pkt.extract(h.h1);
            m.hdr = 0;
            transition select (h.h1.f1[0:0]) {
                0 : parse_h2;
                1 : parse_h3;
            }
        }
        state parse_h2 {
            pkt.extract(h.h2);
            transition accept;
        }
        state parse_h3 {
            pkt.extract(h.h3);
            transition accept;
        }
    )";
    std::string control_body = R"(
        apply {
            if (h.h2.isValid()) {
                if (h.h1.f1[31:31] == 1) {
                    h.h2.f1 = 8w0x12;
                }
            } else {
                h.h1.f1 = 32w0x12345678;
            }

            if (h.h2.isValid()) {
                if (h.h1.f1[31:24] == h.h2.f1)
                    h.h2.f1 = 0;
            } else {
                if (h.h1.f1[7:0] == h.h3.f1)
                    h.h1.f1[7:0] = 8w0xff;
            }
        }
    )";
    std::string deparser_body = R"(
        apply {
            b.emit(h);
        }
    )";

    auto program = make_program(headers, parser_body, control_body, deparser_body);
    auto *defuse = computeDefUse(program, CompilerOptions::FrontendVersion::P4_16);
    ASSERT_TRUE(defuse);

    auto [defs, uses] = get_defs_uses(defuse);

    EXPECT_TRUE(check_def_use(defs, "h.h2.isValid", 2, {0}));
    EXPECT_TRUE(check_def_use(defs, "h.h1.f1[31:31]", 3, {0}));
    EXPECT_TRUE(check_def_use(defs, "h.h2.isValid()", 10, {0}));
    EXPECT_TRUE(check_def_use(defs, "h.h1.f1[31:24]", 11, {0, 7}));
    EXPECT_TRUE(check_def_use(defs, "h.h2.f1", 11, {0, 4}));
    EXPECT_TRUE(check_def_use(defs, "h.h1.f1[7:0]", 14, {0, 7}));
    EXPECT_TRUE(check_def_use(defs, "h.h3.f1", 14, {0}));
    EXPECT_TRUE(check_def_use(defs, "inout ParsedHeaders h", 0, {0, 4, 7, 12, 15}));

    EXPECT_TRUE(check_def_use(uses, "h.h1.f1", 7, {0, 11, 14}));
    EXPECT_TRUE(check_def_use(uses, "h.h2.f1", 4, {0, 11}));
    EXPECT_TRUE(check_def_use(uses, "h.h1.f1[7:0]", 15, {0}));
    EXPECT_TRUE(check_def_use(uses, "h.h2.f1", 12, {0}));
    EXPECT_TRUE(check_def_use(uses, "inout ParsedHeaders h", 0, {0, 2, 3, 10, 11, 14}));
}

}  // namespace Test
