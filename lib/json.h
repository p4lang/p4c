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

#ifndef LIB_JSON_H_
#define LIB_JSON_H_

#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#ifdef P4C_GTEST_ENABLED
#include "gtest/gtest_prod.h"
#endif
#include "lib/big_int.h"
#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/map.h"
#include "lib/string_map.h"

namespace p4c::Test {
class TestJson;
}

namespace p4c::Util {

class IJson : public ICastable {
 public:
    virtual ~IJson() {}
    virtual void serialize(std::ostream &out) const = 0;
    cstring toString() const;
    void dump() const;

    DECLARE_TYPEINFO(IJson);
};

class JsonValue final : public IJson {
#ifdef P4C_GTEST_ENABLED
    FRIEND_TEST(Util, Json);
#endif

 public:
    enum Kind { String, Number, True, False, Null };
    JsonValue() : tag(Kind::Null) {}
    JsonValue(bool b) : tag(b ? Kind::True : Kind::False) {}     // NOLINT
    JsonValue(big_int v) : tag(Kind::Number), value(v) {}        // NOLINT
    JsonValue(int v) : tag(Kind::Number), value(v) {}            // NOLINT
    JsonValue(long v) : tag(Kind::Number), value(v) {}           // NOLINT
    JsonValue(long long v);                                      // NOLINT
    JsonValue(unsigned v) : tag(Kind::Number), value(v) {}       // NOLINT
    JsonValue(unsigned long v) : tag(Kind::Number), value(v) {}  // NOLINT
    JsonValue(unsigned long long v);                             // NOLINT
    JsonValue(double v) : tag(Kind::Number), value(v) {}         // NOLINT
    JsonValue(float v) : tag(Kind::Number), value(v) {}          // NOLINT
    JsonValue(cstring s) : tag(Kind::String), str(s) {}          // NOLINT
    // FIXME: replace these two ctors with std::string view, cannot do now as
    // std::string is implicitly convertible to cstring
    JsonValue(const char *s) : tag(Kind::String), str(s) {}         // NOLINT
    JsonValue(const std::string &s) : tag(Kind::String), str(s) {}  // NOLINT
    void serialize(std::ostream &out) const override;

    bool operator==(const big_int &v) const;
    // is_integral is true for bool
    template <typename T, typename std::enable_if_t<std::is_integral_v<T>, int> = 0>
    bool operator==(const T &v) const {
        return (tag == Kind::Number) && (v == value);
    }
    bool operator==(const double &v) const;
    bool operator==(const float &v) const;
    bool operator==(const cstring &s) const;
    // FIXME: replace these two methods with std::string view, cannot do now as
    // std::string is implicitly convertible to cstring
    bool operator==(const char *s) const;
    bool operator==(const std::string &s) const;
    bool operator==(const JsonValue &other) const;

    bool isNumber() const { return tag == Kind::Number; }
    bool isBool() const { return tag == Kind::True || tag == Kind::False; }
    bool isString() const { return tag == Kind::String; }
    bool isNull() const { return tag == Kind::Null; }

    bool getBool() const;
    cstring getString() const;
    big_int getValue() const;
    int getInt() const;

    static JsonValue *null;

 private:
    JsonValue(Kind kind) : tag(kind) {  // NOLINT
        if (kind == Kind::String || kind == Kind::Number)
            throw std::logic_error("Incorrect constructor called");
    }

    const Kind tag;
    const big_int value = 0;
    const cstring str = cstring::empty;

    DECLARE_TYPEINFO(JsonValue, IJson);
};

class JsonArray final : public IJson, public std::vector<IJson *> {
    friend class Test::TestJson;

 public:
    void serialize(std::ostream &out) const override;
    JsonArray *clone() const { return new JsonArray(*this); }
    JsonArray *append(IJson *value);
    JsonArray *append(big_int v) {
        append(new JsonValue(v));
        return this;
    }
    template <typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    JsonArray *append(T v) {
        append(new JsonValue(v));
        return this;
    }
    JsonArray *append(double v) {
        append(new JsonValue(v));
        return this;
    }
    JsonArray *append(float v) {
        append(new JsonValue(v));
        return this;
    }
    JsonArray *append(cstring s) {
        append(new JsonValue(s));
        return this;
    }
    // FIXME: replace these two methods with std::string view, cannot do now as
    // std::string is implicitly convertible to cstring
    JsonArray *append(const char *s) {
        append(new JsonValue(s));
        return this;
    }
    JsonArray *append(const std::string &s) {
        append(new JsonValue(s));
        return this;
    }
    JsonArray *concatenate(const Util::JsonArray *other) {
        for (auto v : *other) append(v);
        return this;
    }
    JsonArray(std::initializer_list<IJson *> data) : std::vector<IJson *>(data) {}  // NOLINT
    JsonArray() = default;
    JsonArray(std::vector<IJson *> &data) : std::vector<IJson *>(data) {}  // NOLINT

    DECLARE_TYPEINFO(JsonArray, IJson);
};

class JsonObject final : public IJson, public string_map<IJson *> {
    friend class Test::TestJson;

    using base = string_map<IJson *>;

 public:
    JsonObject() = default;
    void serialize(std::ostream &out) const override;
    JsonObject *emplace_non_null(cstring label, IJson *value);

    JsonObject *emplace(cstring label, IJson *value);
    JsonObject *emplace(std::string_view label, IJson *value);

    template <class T, class String>
    auto emplace(String label,
                 T &&s) -> std::enable_if_t<!std::is_convertible_v<T, IJson *>, JsonObject *> {
        emplace(label, new JsonValue(std::forward<T>(s)));
        return this;
    }

    IJson *get(cstring label) const { return ::p4c::get(*this, label); }
    IJson *get(std::string_view label) const { return ::p4c::get(*this, label); }
    template <class T, class S>
    T *getAs(S label) const {
        return get(label)->template to<T>();
    }

    DECLARE_TYPEINFO(JsonObject, IJson);
};

}  // namespace p4c::Util

#endif /* LIB_JSON_H_ */
