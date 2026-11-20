// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <streambuf>
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
    auto result = JsonData::parse(input, true);

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
        EXPECT_THROW(JsonData::parse(input, true), JsonData::error) << invalid;
    }
}

TEST(JSON, StrictParserScalarsAtEofAndDecimalIntegers) {
    for (const char *valid : {"0", "-0", "-12", "12345678901234567890", "true", "false", "null"}) {
        std::istringstream input(valid);
        std::unique_ptr<JsonData> result;
        EXPECT_NO_THROW(result = JsonData::parse(input, true)) << valid;
        EXPECT_NE(result, nullptr) << valid;
        EXPECT_FALSE(input.fail()) << valid;
    }
    std::istringstream input("[12, -34, 0]");
    auto result = JsonData::parse(input, true);
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
                                "true_",
                                "false0",
                                "nullx",
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
        EXPECT_THROW(JsonData::parse(input, true), JsonData::error) << invalid;
    }
    for (auto state : {std::ios::failbit, std::ios::badbit, std::ios::eofbit}) {
        std::istringstream input("[]");
        input.setstate(state);
        EXPECT_THROW(JsonData::parse(input, true), JsonData::error);
    }
}

TEST(JSON, StrictParserUnicodeBoundariesAndSingleValueContract) {
    std::istringstream input(R"("\u0000\uD7FF\uE000\uFFFF\uDBFF\uDFFF" trailing)");
    auto result = JsonData::parse(input, true);
    ASSERT_TRUE(result->is<JsonString>());
    EXPECT_EQ(*result->to<JsonString>(),
              std::string("\0", 1) + "\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbf\xf4\x8f\xbf\xbf");
    // Like operator>>, JsonData::parse extracts one value. Document consumers must
    // check the remaining stream; the trailing text is left for that check.
    std::string trailing;
    input >> trailing;
    EXPECT_EQ(trailing, "trailing");
    std::istringstream legacy("[1,]");
    result = JsonData::parse(legacy, false);
    ASSERT_TRUE(result->is<JsonVector>());
    EXPECT_EQ(result->to<JsonVector>()->size(), 1u);
}

TEST(JSON, StrictAndLegacyStringEscapes) {
    std::istringstream valid(R"("\"\\\/\b\f\n\r\t\u0041\\uD800")");
    auto result = JsonData::parse(valid, true);
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->is<JsonString>());
    EXPECT_EQ(*result->to<JsonString>(), "\"\\/\b\f\n\r\tA\\uD800");

    // C escapes remain available to legacy consumers, independently of strict JSON.
    for (const auto &[escape, decoded] : {std::pair{R"(\v)", "\v"},
                                          {R"(\x41)", "A"},
                                          {R"(\101)", "A"},
                                          {R"(\U0001F600)", "😀"},
                                          {R"(\a)", "\a"},
                                          {R"(\')", "'"},
                                          {R"(\?)", "?"}}) {
        SCOPED_TRACE(escape);
        const auto quoted = std::string("\"") + escape + "\"";
        std::istringstream strict(quoted);
        EXPECT_THROW(JsonData::parse(strict, true), JsonData::error);
        std::istringstream legacy(quoted);
        auto value = JsonData::parse(legacy, false);
        ASSERT_TRUE(value);
        ASSERT_TRUE(value->is<JsonString>());
        EXPECT_EQ(*value->to<JsonString>(), decoded);
    }

    for (char control = 0; control < 0x20; ++control) {
        SCOPED_TRACE(static_cast<int>(control));
        const auto quoted = std::string("\"") + control + "\"";
        std::istringstream strict(quoted);
        EXPECT_THROW(JsonData::parse(strict, true), JsonData::error);
        std::istringstream legacy(quoted);
        auto value = JsonData::parse(legacy, false);
        ASSERT_TRUE(value);
        ASSERT_TRUE(value->is<JsonString>());
        EXPECT_EQ(*value->to<JsonString>(), std::string(1, control));
    }
}

TEST(JSON, LegacyIntegerAtEofTerminates) {
    std::istringstream input("12");
    auto result = JsonData::parse(input, false);
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->is<JsonNumber>());
    EXPECT_EQ(result->to<JsonNumber>()->val, 12);
    EXPECT_FALSE(input.fail());
    EXPECT_TRUE(input.eof());
}

TEST(JSON, StrictParserUsesJsonWhitespace) {
    for (const char *invalid : {"\v[]", "[\f1]", "{\v\"key\":1}", "{\"key\"\f:1}"}) {
        std::istringstream input(invalid);
        EXPECT_THROW(JsonData::parse(input, true), JsonData::error);
    }
    std::istringstream input(" \t\r\n[1, \t\r\n2]");
    std::unique_ptr<JsonData> result;
    EXPECT_NO_THROW(result = JsonData::parse(input, true));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->is<JsonVector>());
}

TEST(JSON, StrictParserRejectsUnrepresentableObjectKeys) {
    // JsonObject uses cstring keys; accepting NUL here would silently truncate
    // the key and could overwrite a distinct member. String values retain NULs.
    std::istringstream input(R"({"a\u0000b":1,"a":2})");
    EXPECT_THROW(JsonData::parse(input, true), JsonData::error);
}

TEST(JSON, StrictParserValidatesRawUtf8) {
    for (const std::string bytes :
         {"\x80", "\xbf", "\xc0\x80", "\xc1\xbf", "\xc2", "\xc2x", "\xe0\x80\x80", "\xe0\x9f\xbf",
          "\xed\xa0\x80", "\xed\xbf\xbf", "\xe2\x82", "\xe2(\xa1", "\xf0\x80\x80\x80",
          "\xf0\x8f\xbf\xbf", "\xf4\x90\x80\x80", "\xf5\x80\x80\x80", "\xff"}) {
        SCOPED_TRACE(bytes);
        std::istringstream value("\"" + bytes + "\"");
        EXPECT_THROW(JsonData::parse(value, true), JsonData::error);
        std::istringstream key("{\"" + bytes + "\":1}");
        EXPECT_THROW(JsonData::parse(key, true), JsonData::error);
        // The stricter encoding contract must not change legacy byte strings.
        std::istringstream legacy("\"" + bytes + "\"");
        auto result = JsonData::parse(legacy, false);
        ASSERT_TRUE(result);
        ASSERT_TRUE(result->is<JsonString>());
        EXPECT_EQ(*result->to<JsonString>(), bytes);
    }
    for (const std::string bytes :
         {"\x7f", "\xc2\x80", "\xdf\xbf", "\xe0\xa0\x80", "\xed\x9f\xbf", "\xee\x80\x80",
          "\xef\xbf\xbf", "\xf0\x90\x80\x80", "\xf4\x8f\xbf\xbf", "é😀"}) {
        SCOPED_TRACE(bytes);
        std::istringstream input("\"" + bytes + "\"");
        auto result = JsonData::parse(input, true);
        ASSERT_TRUE(result);
        ASSERT_TRUE(result->is<JsonString>());
        EXPECT_EQ(*result->to<JsonString>(), bytes);
    }
}

TEST(JSON, ParserLocationQueriesPreserveStreamState) {
    for (const char *text : {"true", "false", "null", "12", "\"text\"", "[]", "{}"}) {
        SCOPED_TRACE(text);
        std::istringstream input(text);
        input.exceptions(std::ios::failbit | std::ios::badbit);
        EXPECT_NO_THROW({
            auto result = JsonData::parse(input, true);
            EXPECT_NE(result, nullptr);
            EXPECT_FALSE(input.fail());
        });
    }
    std::istream missingBuffer(nullptr);
    EXPECT_THROW(JsonData::parse(missingBuffer, true), JsonData::error);
}

TEST(JSON, PerCallModeAndExtractionCompatibility) {
    // Restore the global even when an assertion or unexpected exception exits.
    struct RestoreMode {
        bool saved = JsonData::strict;
        ~RestoreMode() { JsonData::strict = saved; }
    } restore;
    for (bool global : {false, true}) {
        JsonData::strict = global;
        std::istringstream strict("[1,]");
        EXPECT_THROW(JsonData::parse(strict, true), JsonData::error);
        EXPECT_EQ(JsonData::strict, global);
        std::istringstream legacy("[1,]");
        auto value = JsonData::parse(legacy, false);
        ASSERT_TRUE(value);
        EXPECT_EQ(JsonData::strict, global);
        std::istringstream extracted("[1,]");
        if (global)
            EXPECT_THROW(extracted >> value, JsonData::error);
        else
            EXPECT_NO_THROW(extracted >> value);

        auto *original = value.get();
        std::istringstream invalid("[1,]");
        JsonData::strict = true;
        EXPECT_THROW(invalid >> value, JsonData::error);
        EXPECT_EQ(value.get(), original);
        std::istringstream noValue("?");
        JsonData::strict = false;
        EXPECT_EQ(&(noValue >> value), &noValue);
        JsonData::strict = global;
        EXPECT_EQ(value.get(), original);
        EXPECT_EQ(noValue.peek(), '?');
        std::istringstream eof;
        eof.setstate(std::ios::eofbit);
        EXPECT_EQ(JsonData::parse(eof, false), nullptr);
        EXPECT_EQ(JsonData::strict, global);
    }
}

namespace {

class NonSeekingJsonBuffer : public std::stringbuf {
 public:
    using std::stringbuf::stringbuf;

 protected:
    pos_type seekoff(off_type, std::ios::seekdir, std::ios::openmode) override {
        return pos_type(off_type(-1));
    }
    pos_type seekpos(pos_type, std::ios::openmode) override { return pos_type(off_type(-1)); }
};

// A one-character get area exercises refills without requiring seeking.
class SingleCharacterJsonBuffer : public std::streambuf {
    std::string bytes;
    size_t offset = 0;
    char slot = 0;

 public:
    explicit SingleCharacterJsonBuffer(std::string text) : bytes(std::move(text)) {}

 protected:
    int_type underflow() override {
        if (gptr() && gptr() < egptr()) return traits_type::to_int_type(*gptr());
        if (offset == bytes.size()) return traits_type::eof();
        slot = bytes[offset++];
        setg(&slot, &slot, &slot + 1);
        return traits_type::to_int_type(slot);
    }
};

}  // namespace

TEST(JSON, ParserSupportsNonSeekingStreams) {
    for (bool strict : {false, true}) {
        NonSeekingJsonBuffer buffer(R"({"a":[true,false,null,12,"é"]})");
        std::istream input(&buffer);
        input.exceptions(std::ios::failbit | std::ios::badbit);
        auto result = JsonData::parse(input, strict);
        ASSERT_TRUE(result);
        EXPECT_TRUE(result->is<JsonObject>());
        EXPECT_FALSE(input.fail());
        JsonData::LocationInfo locations("pipe", input);
        EXPECT_EQ(locations.desc(*result), "");
    }
}

TEST(JSON, StrictTokensLeaveDelimitersAcrossRefills) {
    for (const char *text : {"true,", "false]", "null}", "true", "false", "null"}) {
        SCOPED_TRACE(text);
        for (auto mask : {std::ios::goodbit, std::ios::failbit | std::ios::badbit,
                          std::ios::eofbit | std::ios::failbit | std::ios::badbit}) {
            SCOPED_TRACE(mask);
            SingleCharacterJsonBuffer buffer(text);
            std::istream input(&buffer);
            input.exceptions(mask);
            auto last = std::string(text).back();
            bool delimiter = last == ',' || last == ']' || last == '}';
            if (!delimiter && (mask & std::ios::eofbit)) {
                EXPECT_THROW(JsonData::parse(input, true), std::ios_base::failure);
                EXPECT_TRUE(input.eof());
                EXPECT_FALSE(input.fail());
            } else {
                auto result = JsonData::parse(input, true);
                ASSERT_TRUE(result);
                EXPECT_FALSE(input.fail());
                EXPECT_EQ(input.eof(), !delimiter);
                if (delimiter) {
                    EXPECT_EQ(input.get(), last);
                }
            }
        }
    }
}

TEST(JSON, StrictParserRejectsUnicodeWhitespaceOutsideStrings) {
    // Includes every sequence recognized by PR #5935's JSON parser.
    for (const std::string space :
         {"\xc2\xa0", "\xe2\x80\x80", "\xe2\x80\x81", "\xe2\x80\x82", "\xe2\x80\x83",
          "\xe2\x80\x87", "\xe2\x80\x88", "\xe2\x80\x89", "\xe2\x80\x8a", "\xe2\x80\xaf",
          "\xe2\x81\xa0", "\xe3\x80\x80", "\xef\xbb\xbf"}) {
        for (const std::string &text :
             {space + "[]", "[" + space + "1]", "[1," + space + "2]", "[1" + space + "]",
              "{\"a\"" + space + ":1}", "{\"a\":" + space + "1}"}) {
            SCOPED_TRACE(text);
            std::istringstream input(text);
            EXPECT_THROW(JsonData::parse(input, true), JsonData::error);
        }
        std::istringstream quoted("\"" + space + "\"");
        auto result = JsonData::parse(quoted, true);
        ASSERT_TRUE(result);
        EXPECT_EQ(*result->to<JsonString>(), space);
    }
}

TEST(JSON, StrictParserRejectsNonStringObjectKeys) {
    for (const char *text : {"{true:1}", "{null:1}", "{[]:1}", "{{}:1}"}) {
        std::istringstream input(text);
        EXPECT_THROW(JsonData::parse(input, true), JsonData::error);
    }
}
