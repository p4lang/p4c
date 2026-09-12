// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "lib/json.h"

#include <gtest/gtest.h>

#include <sstream>

namespace P4::Util {

template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
static cstring getNumStringRepr(T v) {
    std::ostringstream sstream;
    sstream << std::dec << v;
    return cstring(sstream.str());
}

TEST(Util, Json) {
    IJson *value;
    value = new JsonValue(true);
    EXPECT_EQ("true", value->toString());
    value = new JsonValue();
    EXPECT_EQ("null", value->toString());
    value = new JsonValue(JsonValue::Kind::True);
    EXPECT_EQ("true", value->toString());
    value = new JsonValue("5");
    EXPECT_EQ("\"5\"", value->toString());
    value = new JsonValue(5);
    EXPECT_EQ("5", value->toString());
    value = new JsonValue(static_cast<unsigned long long>(123456789000ULL));
    EXPECT_EQ("123456789000", value->toString());
    value = new JsonValue(static_cast<long long>(123456789000LL));
    EXPECT_EQ("123456789000", value->toString());
    value = new JsonValue(static_cast<long long>(-123456789000LL));
    EXPECT_EQ("-123456789000", value->toString());
    auto smallestLongLong = static_cast<long long>(1LL << 63);
    value = new JsonValue(smallestLongLong);
    EXPECT_EQ(getNumStringRepr(smallestLongLong), value->toString());
    value = new JsonValue(0.0f);
    EXPECT_EQ("0", value->toString());
    value = new JsonValue(3.14f);
    EXPECT_EQ("3.14", value->toString());
    value = new JsonValue(2.718);
    EXPECT_EQ("2.718", value->toString());
    value = new JsonValue(-0.00123);
    EXPECT_EQ("-0.00123", value->toString());
    value = new JsonValue(1.23456e-10);
    EXPECT_EQ("1.23456e-10", value->toString());
    value = new JsonValue(static_cast<float>(1.0 / 3.0));
    std::string floatStr = value->toString().string();
    EXPECT_EQ(floatStr.find("0.333"), 0);

    auto arr = new JsonArray();
    arr->append(5);
    EXPECT_EQ("[5]", arr->toString());
    arr->append("5");
    EXPECT_EQ("[5, \"5\"]", arr->toString());

    auto arr1 = new JsonArray();
    arr->append(arr1);
    EXPECT_EQ("[\n  5,\n  \"5\",\n  []\n]", arr->toString());
    arr1->append(true);
    EXPECT_EQ("[\n  5,\n  \"5\",\n  [true]\n]", arr->toString());

    auto obj = new JsonObject();
    EXPECT_EQ("{\n}", obj->toString());
    obj->emplace("x", "x");
    EXPECT_EQ("{\n  \"x\" : \"x\"\n}", obj->toString());
    obj->emplace("y", arr);
    EXPECT_EQ("{\n  \"x\" : \"x\",\n  \"y\" : [\n    5,\n    \"5\",\n    [true]\n  ]\n}",
              obj->toString());
}

TEST(Util, JsonStringEscaping) {
    // Quotes must be escaped inside JSON strings.
    EXPECT_EQ("\"hello \\\"world\\\"\"", JsonValue("hello \"world\"").toString());

    // Backslashes must be escaped inside JSON strings.
    EXPECT_EQ("\"backslash \\\\ test\"", JsonValue("backslash \\ test").toString());

    // Newlines must be represented as the JSON escape sequence \\n.
    EXPECT_EQ("\"line1\\nline2\"", JsonValue("line1\nline2").toString());

    // Tabs must be represented as the JSON escape sequence \\t.
    EXPECT_EQ("\"column1\\tcolumn2\"", JsonValue("column1\tcolumn2").toString());

    // Other control characters must use the JSON Unicode escape form.
    std::string controlCharacter(1, '\x01');
    EXPECT_EQ("\"\\u0001\"", JsonValue(controlCharacter).toString());

    // Escaping must also work when the string is serialized as a
    // JsonValue contained inside a JsonObject.
    auto obj = new JsonObject();
    obj->emplace("message", "hello \"world\"\n");
    EXPECT_EQ("{\n  \"message\" : \"hello \\\"world\\\"\\n\"\n}", obj->toString());
}

}  // namespace P4::Util
