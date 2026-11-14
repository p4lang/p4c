// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ir/ir.h"
#include "ir/json_loader.h"
#include "ir/json_parser.h"
#include "ir/visitor.h"
#include "lib/json.h"
#include "lib/source_file.h"

using namespace P4;
using namespace P4::literals;

TEST(IR, DumpJSON) {
    auto c = new IR::Constant(2);
    IR::Expression *e1 = new IR::Add(Util::SourceInfo(), c, c);

    std::stringstream ss, ss2;
    JSONGenerator(ss).emit(e1);
    std::cout << ss.str();

    JSONLoader loader(ss);
    std::cout << loader.as<JsonData>();

    const IR::Node *e2 = nullptr;
    loader >> e2;
    JSONGenerator(std::cout).emit(e2);
}

TEST(JSON, string_map) {
    std::vector<string_map<std::string>> data, copy;
    data.resize(2);
    data[0]["x"] = "\ttab";
    data[0]["a"] = "\"";
    data[1]["?"] = "question";
    data[1]["-"] = "dash";
    data[1]["\x1c"] = "esc";

    std::stringstream ss;
    JSONGenerator(ss).emit(data);

    // std::cout << ss.str();

    JSONLoader(ss) >> copy;

    EXPECT_EQ(data.size(), copy.size());
    EXPECT_EQ(data[0], copy[0]);
    EXPECT_EQ(data[1], copy[1]);
}

TEST(JSON, std_map) {
    std::map<big_int, bitvec> data, copy;
    data[big_int(1) << 100].setrange(100, 100);
    data[1] = bitvec(1);

    std::stringstream ss;
    JSONGenerator(ss).emit(data);

    // std::cout << ss.str();

    JSONLoader(ss) >> copy;

    EXPECT_EQ(data, copy);
}

TEST(IR, SourceInfoJsonEscaping) {
    // No quotes: quoted lines take a separate path in getBriefSourceFragment.
    Util::InputSources sources;
    sources.appendText("foo(a\\b);");
    sources.mapLine("test.p4", 1);
    sources.appendText("\n");
    sources.seal();

    Util::SourceInfo si(&sources, Util::SourcePosition(1, 0), Util::SourcePosition(1, 9));
    ASSERT_TRUE(si.isValid());
    IR::Constant c(si, big_int(2));

    std::unique_ptr<Util::JsonObject> obj(c.sourceInfoJsonObj());
    ASSERT_NE(obj, nullptr);

    const cstring raw = "foo(a\\b);"_cs;
    EXPECT_EQ(raw, obj->getAs<Util::JsonValue>("source_fragment")->getString());

    const std::string serialized = obj->toString().string();
    EXPECT_NE(std::string::npos, serialized.find("\"foo(a\\\\b);\""));
    EXPECT_EQ(std::string::npos, serialized.find("\"foo(a\\\\\\\\b);\""));
}

TEST(JSON, variant) {
    std::vector<std::variant<int, std::string>> data, copy;
    data.emplace_back(2);
    data.emplace_back(10);
    data.emplace_back("foobar");
    data.emplace_back(10);
    data.emplace_back(1);
    data.emplace_back("x");

    std::stringstream ss;
    JSONGenerator(ss).emit(data);

    // std::cout << ss.str();

    JSONLoader(ss) >> copy;

    EXPECT_EQ(data.size(), copy.size());
    for (size_t i = 0; i < data.size() && i < copy.size(); ++i) {
        EXPECT_EQ(data[i], copy[i]);
    }
}

TEST(JSON, IntegralRoundTrip) {
    // Integers wider than int must survive a JSON round trip exactly.
    const std::vector<unsigned long long> values = {
        0ULL, 1ULL, 2147483647ULL, 2147483648ULL, 8589934591ULL, 18446744073709551615ULL};
    std::stringstream ss;
    JSONGenerator(ss).emit(values);
    std::vector<unsigned long long> result;
    JSONLoader(ss) >> result;
    EXPECT_EQ(values, result);
}

// Extracted from the structured diagnostics tests in PR #5940.
// Test the JSON parser directly, without the preprocessor dependency.
TEST(JSON, StrictParserUnicodeAndLegacyModeIsolation) {
    const bool previous = JsonData::strict;
    std::istringstream input(
        R"([{"kind":"note","message":"\uD83D\uDE00\/\u00E9","locations":[]}])");
    std::unique_ptr<JsonData> result;
    parseJson(input, result, true);

    ASSERT_NE(result, nullptr);
    auto *records = result->to<JsonVector>();
    ASSERT_NE(records, nullptr);
    ASSERT_EQ(records->size(), 1U);

    auto *record = records->at(0)->to<JsonObject>();
    ASSERT_NE(record, nullptr);
    auto *message = record->at("message")->to<JsonString>();
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(*message, "😀/é");
    EXPECT_EQ(JsonData::strict, previous);
}

TEST(JSON, StrictParserRejectsMalformedInput) {
    // Reuse malformed cases from PR #5940's preprocessor regression test.
    for (const char *invalid : {
             "[{},]",
             "[1 2]",
             R"([{"kind":"note","message":"bad\v","locations":[]}])",
         }) {
        std::istringstream input(invalid);
        std::unique_ptr<JsonData> result;
        EXPECT_THROW(parseJson(input, result, true), JsonData::error) << invalid;
    }
}

TEST(JSON, StrictParserScalarsAtEofAndDecimalIntegers) {
    for (const char *valid : {"0", "-0", "-12", "12345678901234567890", "true", "false", "null"}) {
        std::istringstream input(valid);
        std::unique_ptr<JsonData> result;
        EXPECT_NO_THROW(parseJson(input, result, true)) << valid;
        EXPECT_NE(result, nullptr) << valid;
        EXPECT_FALSE(input.fail()) << valid;
    }
    std::istringstream input("[12, -34, 0]");
    std::unique_ptr<JsonData> result;
    parseJson(input, result, true);
    ASSERT_TRUE(result->is<JsonVector>());
    const auto &numbers = *result->to<JsonVector>();
    ASSERT_EQ(numbers.size(), 3u);
    EXPECT_EQ(numbers[0]->to<JsonNumber>()->val, 12);
    EXPECT_EQ(numbers[1]->to<JsonNumber>()->val, -34);
}

TEST(JSON, StrictParserRejectsIncompleteValuesAndInvalidTokens) {
    for (const char *invalid : {"",
                                " ",
                                "[",
                                "{",
                                "[1",
                                "{\"a\":1",
                                "[1,",
                                "{\"a\":",
                                "[1,]",
                                "{\"a\":1,}",
                                "[1 2]",
                                "{\"a\" 1}",
                                "{1:2}",
                                "[true false]",
                                "-",
                                "01",
                                "-01",
                                "1.5",
                                "1e2",
                                "1e+",
                                "1x",
                                "True",
                                "NULL",
                                "tru",
                                "\"unterminated",
                                "\"bad\\\"",
                                "\"\\uD800\"",
                                "\"\\uDC00\"",
                                "\"\\uD800\\u0041\"",
                                "\"\\uD800\\uDBFF\"",
                                "\"\\u12\"",
                                "\"\\uZZZZ\"",
                                "\"\\x41\"",
                                "\"\\v\"",
                                "\"\n\""}) {
        std::istringstream input(invalid);
        std::unique_ptr<JsonData> result;
        EXPECT_THROW(parseJson(input, result, true), JsonData::error) << invalid;
    }
    for (auto state : {std::ios::failbit, std::ios::badbit, std::ios::eofbit}) {
        std::istringstream input("[]");
        input.setstate(state);
        std::unique_ptr<JsonData> result;
        EXPECT_THROW(parseJson(input, result, true), JsonData::error);
    }
}

TEST(JSON, StrictParserUnicodeBoundariesAndSingleValueContract) {
    std::istringstream input(R"("\u0000\uD7FF\uE000\uFFFF\uDBFF\uDFFF" trailing)");
    std::unique_ptr<JsonData> result;
    parseJson(input, result, true);
    ASSERT_TRUE(result->is<JsonString>());
    EXPECT_EQ(*result->to<JsonString>(),
              std::string("\0", 1) + "\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbf\xf4\x8f\xbf\xbf");
    // Like operator>>, parseJson extracts one value. Document consumers must
    // check the remaining stream; the trailing text is left for that check.
    std::string trailing;
    input >> trailing;
    EXPECT_EQ(trailing, "trailing");
    std::istringstream legacy("[1,]");
    parseJson(legacy, result, false);
    ASSERT_TRUE(result->is<JsonVector>());
    EXPECT_EQ(result->to<JsonVector>()->size(), 1u);
}

TEST(JSON, LegacyIntegerAtEofTerminates) {
    std::istringstream input("12");
    std::unique_ptr<JsonData> result;
    parseJson(input, result, false);
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->is<JsonNumber>());
    EXPECT_EQ(result->to<JsonNumber>()->val, 12);
}

TEST(JSON, StrictParserUsesJsonWhitespace) {
    for (const char *invalid : {"\v[]", "[\f1]", "{\v\"key\":1}", "{\"key\"\f:1}"}) {
        std::istringstream input(invalid);
        std::unique_ptr<JsonData> result;
        EXPECT_THROW(parseJson(input, result, true), JsonData::error);
    }
    std::istringstream input(" \t\r\n[1, \t\r\n2]");
    std::unique_ptr<JsonData> result;
    EXPECT_NO_THROW(parseJson(input, result, true));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->is<JsonVector>());
}

TEST(JSON, StrictParserRejectsUnrepresentableObjectKeys) {
    // JsonObject uses cstring keys; accepting NUL here would silently truncate
    // the key and could overwrite a distinct member. String values retain NULs.
    std::istringstream input(R"({"a\u0000b":1,"a":2})");
    std::unique_ptr<JsonData> result;
    EXPECT_THROW(parseJson(input, result, true), JsonData::error);
}
