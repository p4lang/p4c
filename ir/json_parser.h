#ifndef IR_JSON_PARSER_H_
#define IR_JSON_PARSER_H_

#include <iosfwd>
#include <string>
#include <vector>

#include "lib/big_int_util.h"
#include "lib/castable.h"
#include "lib/cstring.h"
#include "lib/ordered_map.h"

class JsonData : public ICastable {
 public:
    JsonData() {}
    JsonData(const JsonData &) = default;
    JsonData(JsonData &&) = default;
    JsonData &operator=(const JsonData &) & = default;
    JsonData &operator=(JsonData &&) & = default;
    virtual ~JsonData() {}

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

class JsonVector : public JsonData, public std::vector<JsonData *> {
 public:
    JsonVector() {}
    JsonVector(const std::vector<JsonData *> &v)  // NOLINT(runtime/explicit)
        : std::vector<JsonData *>(v) {}
    JsonVector &operator=(const JsonVector &) & = default;
    JsonVector &operator=(JsonVector &&) & = default;

    DECLARE_TYPEINFO(JsonVector, JsonData);
};

class JsonObject : public JsonData, public ordered_map<std::string, JsonData *> {
    bool _hasSrcInfo = true;

 public:
    JsonObject() {}
    JsonObject(const JsonObject &obj) = default;
    JsonObject &operator=(JsonObject &&) & = default;
    JsonObject(const ordered_map<std::string, JsonData *> &v)  // NOLINT(runtime/explicit)
        : ordered_map<std::string, JsonData *>(v) {}
    int get_id() const;
    std::string get_type() const;
    std::string get_filename() const;
    std::string get_sourceFragment() const;
    int get_line() const;
    int get_column() const;
    JsonObject get_sourceJson() const;
    bool hasSrcInfo() { return _hasSrcInfo; }
    void setSrcInfo(bool value) { _hasSrcInfo = value; }

    DECLARE_TYPEINFO(JsonObject, JsonData);
};

class JsonNull : public JsonData {
    DECLARE_TYPEINFO(JsonNull, JsonData);
};

std::string getIndent(int l);

std::ostream &operator<<(std::ostream &out, JsonData *json);
std::istream &operator>>(std::istream &in, JsonData *&json);

#endif /* IR_JSON_PARSER_H_ */
