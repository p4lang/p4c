#ifndef IR_JSON_PARSER_H_
#define IR_JSON_PARSER_H_

#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "lib/big_int_util.h"
#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/string_map.h"

namespace P4 {

class JsonData : public ICastable {
    std::streamoff start = -1, finish = -1;  // location in istream read from

 protected:
    JsonData() {}
    JsonData(const JsonData &) = default;
    JsonData(JsonData &&) = default;
    JsonData &operator=(const JsonData &) = default;
    JsonData &operator=(JsonData &&) = default;

 public:
    virtual ~JsonData() = default;
    static bool strict;  // enforce strict syntax checking of json on input, default false.

    friend std::istream &operator>>(std::istream &in, std::unique_ptr<JsonData> &json);
    struct error : public std::runtime_error {
        std::streamoff loc;
        error(const char *what, std::streamoff l) : runtime_error(what), loc(l) {}
        error(const std::string &what, std::streamoff l) : runtime_error(what), loc(l) {}
    };

    class LocationInfo {
        std::string name;
        std::istream &in;
        std::streamoff scanned = 0;
        std::map<std::streamoff, int> line;  // map offset (to start of line) to linenumber
     public:
        LocationInfo(std::string n, std::istream &i) : name(n), in(i) {
            line[0] = 1;  // line 1 at the start of the file
        }
        std::pair<int, int> loc(std::streamoff l);
        std::string desc(std::streamoff l);
        std::string desc(const JsonData &d);
    };

    DECLARE_TYPEINFO(JsonData);
};

class JsonNumber : public JsonData {
 public:
    explicit JsonNumber(big_int v) : val(v) {}
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
    explicit JsonString(std::string_view s) : std::string(s) {}
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
    explicit JsonVector(std::vector<std::unique_ptr<JsonData>> &&v)
        : std::vector<std::unique_ptr<JsonData>>(std::move(v)) {}
    JsonVector &operator=(const JsonVector &) = delete;
    JsonVector &operator=(JsonVector &&) = default;

    DECLARE_TYPEINFO(JsonVector, JsonData);
};

class JsonObject : public JsonData, public string_map<std::unique_ptr<JsonData>> {
 public:
    JsonObject() {}
    JsonObject(const JsonObject &obj) = delete;
    JsonObject &operator=(JsonObject &&) = default;
    explicit JsonObject(string_map<std::unique_ptr<JsonData>> &&v)
        : string_map<std::unique_ptr<JsonData>>(std::move(v)) {}

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
