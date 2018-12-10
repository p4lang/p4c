#ifndef IR_JSON_PARSER_H_
#define IR_JSON_PARSER_H_

#include <iosfwd>
#include <vector>

#include "lib/cstring.h"
#include "lib/gmputil.h"
#include "lib/ordered_map.h"

class JsonData {
 public:
    JsonData() {}
    JsonData(const JsonData&) = default;
    JsonData(JsonData&&) = default;
    JsonData &operator=(const JsonData &) & = default;
    JsonData &operator=(JsonData &&) & = default;
    template<typename T> bool is() const { return to<T>() != nullptr; }
    template<typename T> const T* to() const { return dynamic_cast<const T*>(this); }
    virtual ~JsonData() {}
};

class JsonNumber : public JsonData {
 public:
    JsonNumber(mpz_class v) : val(v) {}   // NOLINT(runtime/explicit)
    operator int() const { return val.get_si(); }  // Does not handle overflow
    mpz_class val;
};

class JsonBoolean : public JsonData {
 public:
    JsonBoolean(bool v) : val(v) {}   // NOLINT(runtime/explicit)
    operator bool() const { return val; }
    bool val;
};

class JsonString : public JsonData, public std::string {
 public:
    JsonString() {}
    JsonString(const std::string &s) : std::string(s) {}    // NOLINT(runtime/explicit)
    JsonString(const char *s) : std::string(s) {}           // NOLINT(runtime/explicit)
    JsonString(const JsonString&) = default;
    JsonString(JsonString&&) = default;
    JsonString &operator=(const JsonString&) & = default;
    JsonString &operator=(JsonString&&) & = default;
    operator cstring() { return cstring(this->c_str()); }
};

class JsonVector : public JsonData, public std::vector<JsonData*> {
 public:
    JsonVector() {}
    JsonVector(const std::vector<JsonData*> & v)   // NOLINT(runtime/explicit)
    : std::vector<JsonData*>(v) {}
    JsonVector &operator=(const JsonVector&) & = default;
    JsonVector &operator=(JsonVector&&) & = default;
};

class JsonObject : public JsonData, public ordered_map<std::string, JsonData*>  {
 public:
    JsonObject &operator=(JsonObject&&) & = default;
    JsonObject(const ordered_map<std::string, JsonData*> & v)  // NOLINT(runtime/explicit)
    : ordered_map<std::string, JsonData*>(v) {}
    int get_id() const;
    std::string get_type() const;
};

class JsonNull : public JsonData {};

std::string getIndent(int l);

std::ostream& operator<<(std::ostream &out, JsonData* json);
std::istream& operator>>(std::istream &in, JsonData*& json);

#endif /* IR_JSON_PARSER_H_ */
