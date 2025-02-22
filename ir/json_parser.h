#ifndef IR_JSON_PARSER_H_
#define IR_JSON_PARSER_H_

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "lib/big_int_util.h"
#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

namespace P4 {

class JsonData : public ICastable {
 protected:
    JsonData() {}
    JsonData(const JsonData &) = default;
    JsonData(JsonData &&) = default;
    JsonData &operator=(const JsonData &) = default;
    JsonData &operator=(JsonData &&) = default;

 public:
    virtual ~JsonData() = default;

    DECLARE_TYPEINFO(JsonData);
};

class JsonNumber : public JsonData {
 public:
    JsonNumber(big_int v) : val(v) {}          // NOLINT(runtime/explicit)
    operator int() const { return int(val); }  // Does not handle overflow
    big_int val;

    DECLARE_TYPEINFO(JsonNumber, JsonData);
};

class JsonBoolean : public JsonData {
 public:
    JsonBoolean(bool v) : val(v) {}  // NOLINT(runtime/explicit)
    operator bool() const { return val; }
    bool val;

    DECLARE_TYPEINFO(JsonBoolean, JsonData);
};

class JsonString : public JsonData, public std::string {
 public:
    JsonString() {}
    JsonString(const std::string &s) : std::string(s) {}  // NOLINT(runtime/explicit)
    JsonString(const char *s) : std::string(s) {}         // NOLINT(runtime/explicit)
    JsonString(const JsonString &) = default;
    JsonString(JsonString &&) = default;
    JsonString &operator=(const JsonString &) & = default;
    JsonString &operator=(JsonString &&) & = default;
    operator cstring() { return cstring(this->c_str()); }

    DECLARE_TYPEINFO(JsonString, JsonData);
};

class JsonVector : public JsonData, public std::vector<std::unique_ptr<JsonData>> {
 public:
    JsonVector() {}
    JsonVector(std::vector<std::unique_ptr<JsonData>> &&v)  // NOLINT(runtime/explicit)
        : std::vector<std::unique_ptr<JsonData>>(std::move(v)) {}
    JsonVector &operator=(const JsonVector &) = delete;
    JsonVector &operator=(JsonVector &&) = default;

    DECLARE_TYPEINFO(JsonVector, JsonData);
};

class JsonObject : public JsonData, public ordered_map<std::string, std::unique_ptr<JsonData>> {
    // bool _hasSrcInfo = true;

 public:
    JsonObject() {}
    JsonObject(const JsonObject &obj) = delete;
    JsonObject &operator=(JsonObject &&) = default;
    JsonObject(ordered_map<std::string, std::unique_ptr<JsonData>> &&v)  // NOLINT(runtime/explicit)
        : ordered_map<std::string, std::unique_ptr<JsonData>>(std::move(v)) {}

    DECLARE_TYPEINFO(JsonObject, JsonData);
};

class JsonNull : public JsonData {
    DECLARE_TYPEINFO(JsonNull, JsonData);
};

std::string getIndent(int l);

std::ostream &operator<<(std::ostream &out, const JsonData *json);
inline std::ostream &operator<<(std::ostream &out, const JsonData &json) { return out << &json; }
std::istream &operator>>(std::istream &in, std::unique_ptr<JsonData> &json);

}  // namespace P4

#endif /* IR_JSON_PARSER_H_ */
