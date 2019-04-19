/*
Copyright 2013-present Barefoot Networks, Inc. 

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

#include <sstream>

#include "gtest/gtest.h"
#include "lib/json.h"

namespace Util {

template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
static cstring getNumStringRepr(T v) {
    std::ostringstream sstream;
    sstream << std::dec << v;
    return cstring(sstream.str());
}

TEST(Util, Json) {
    IJson* value;
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

}  // namespace Util
