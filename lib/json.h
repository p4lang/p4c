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

#ifndef _LIB_JSON_H_
#define _LIB_JSON_H_

#include <iostream>
#include <stdexcept>
#include <vector>
#include <type_traits>

#include "gtest/gtest_prod.h"
#include "lib/gmputil.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

namespace Test { class TestJson; }

namespace Util {

class IJson {
 public:
    virtual ~IJson() {}
    virtual void serialize(std::ostream& out) const = 0;
    cstring toString() const;
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> T* to() { return dynamic_cast<T*>(this); }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
    void dump() const;
};

class JsonValue final : public IJson {
    FRIEND_TEST(Util, Json);

 public:
    enum Kind {
        String,
        Number,
        True,
        False,
        Null
    };
    JsonValue() : tag(Kind::Null) {}
    JsonValue(bool b) : tag(b ? Kind::True : Kind::False) {}          // NOLINT
    JsonValue(mpz_class v) : tag(Kind::Number), value(v) {}           // NOLINT
    JsonValue(int v) : tag(Kind::Number), value(v) {}                 // NOLINT
    JsonValue(long v) : tag(Kind::Number), value(v) {}                // NOLINT
    JsonValue(long long v);                                           // NOLINT
    JsonValue(unsigned v) : tag(Kind::Number), value(v) {}            // NOLINT
    JsonValue(unsigned long v) : tag(Kind::Number), value(v) {}       // NOLINT
    JsonValue(unsigned long long v);                                  // NOLINT
    JsonValue(double v) : tag(Kind::Number), value(v) {}              // NOLINT
    JsonValue(float v) : tag(Kind::Number), value(v) {}               // NOLINT
    JsonValue(cstring s) : tag(Kind::String), str(s) {}               // NOLINT
    JsonValue(const std::string &s) : tag(Kind::String), str(s) {}    // NOLINT
    JsonValue(const char* s) : tag(Kind::String), str(s) {}           // NOLINT
    void serialize(std::ostream& out) const;

    bool operator==(const mpz_class& v) const;
    // is_integral is true for bool
    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    bool operator==(const T& v) const
    { return (tag == Kind::Number) && (v == value); }
    bool operator==(const double& v) const;
    bool operator==(const float& v) const;
    bool operator==(const cstring& s) const;
    bool operator==(const std::string& s) const;
    bool operator==(const char* s) const;
    bool operator==(const JsonValue& other) const;

    bool isNumber() const { return tag == Kind::Number; }
    bool isBool() const { return tag == Kind::True || tag == Kind::False; }
    bool isString() const { return tag == Kind::String; }
    bool isNull() const { return tag == Kind::Null; }

    bool getBool() const;
    cstring getString() const;
    mpz_class getValue() const;
    int getInt() const;

    static JsonValue* null;

 private:
    JsonValue(Kind kind) : tag(kind) {                        // NOLINT
        if (kind == Kind::String || kind == Kind::Number)
            throw std::logic_error("Incorrect constructor called");
    }

    static mpz_class makeValue(long long v);
    static mpz_class makeValue(unsigned long long v);

    const Kind tag;
    const mpz_class value = 0;
    const cstring str = nullptr;
};

class JsonArray final : public IJson, public std::vector<IJson*> {
    friend class Test::TestJson;
 public:
    void serialize(std::ostream& out) const;
    JsonArray* append(IJson* value);
    JsonArray* append(mpz_class v) { append(new JsonValue(v)); return this; }
    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    JsonArray* append(T v) { append(new JsonValue(v)); return this; }
    JsonArray* append(double v) { append(new JsonValue(v)); return this; }
    JsonArray* append(float v) { append(new JsonValue(v)); return this; }
    JsonArray* append(cstring s) { append(new JsonValue(s)); return this; }
    JsonArray* append(const std::string &s) { append(new JsonValue(s)); return this; }
    JsonArray* append(const char* s) { append(new JsonValue(s)); return this; }
    JsonArray(std::initializer_list<IJson*> data) : std::vector<IJson*>(data) {} // NOLINT
    JsonArray() = default;
};

class JsonObject final : public IJson, public ordered_map<cstring, IJson*> {
    friend class Test::TestJson;

 public:
    JsonObject() = default;
    void serialize(std::ostream& out) const;
    JsonObject* emplace(cstring label, IJson* value);
    JsonObject* emplace_non_null(cstring label, IJson* value);
    JsonObject* emplace(cstring label, mpz_class v)
    { emplace(label, new JsonValue(v)); return this; }
    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    JsonObject* emplace(cstring label, T v)
    { emplace(label, new JsonValue(v)); return this; }
    JsonObject* emplace(cstring label, float v)
    { emplace(label, new JsonValue(v)); return this; }
    JsonObject* emplace(cstring label, cstring s)
    { emplace(label, new JsonValue(s)); return this; }
    JsonObject* emplace(cstring label, std::string s)
    { emplace(label, new JsonValue(s)); return this; }
    JsonObject* emplace(cstring label, const char* s)
    { emplace(label, new JsonValue(s)); return this; }
    IJson* get(cstring label) const { return ::get(*this, label); }
};

}  // namespace Util

#endif  /* _LIB_JSON_H_ */
