#ifndef IR_JSON_PARSER_H_
#define IR_JSON_PARSER_H_

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>

#include "../lib/ordered_map.h"
class JsonData {
 public:
    JsonData() {}
    JsonData(const JsonData&) = default;
    JsonData(JsonData&&) = default;
    JsonData &operator=(const JsonData &) & = default;
    JsonData &operator=(JsonData &&) & = default;
    virtual ~JsonData() {}
};

class JsonNumber : public JsonData {
 public:
    JsonNumber(long long v) : val(v) {}   // NOLINT(runtime/explicit)
    long long val;
};

class JsonBoolean : public JsonData {
 public:
    JsonBoolean(bool v) : val(v) {}   // NOLINT(runtime/explicit)
    bool val;
};

class JsonString : public JsonData, public std::string {
 public:
    JsonString() {}
    JsonString(const std::string &s) : std::string(s) {}   // NOLINT(runtime/explicit)
    JsonString(const JsonString&) = default;
    JsonString(JsonString&&) = default;
    JsonString &operator=(const JsonString&) & = default;
    JsonString &operator=(JsonString&&) & = default;
};

class JsonVector : public JsonData, public std::vector<JsonData*> {
 public:
    JsonVector() {}
    JsonVector(const std::vector<JsonData*> & v)   // NOLINT(runtime/explicit)
    : std::vector<JsonData*>(v) {}
    JsonVector &operator=(const JsonVector&) & = default;
    JsonVector &operator=(JsonVector&&) & = default;
};

class JsonObject : public JsonData, public ordered_map<JsonString, JsonData*>  {
 public:
    JsonObject &operator=(JsonObject&&) & = default;
    JsonObject(const ordered_map<JsonString, JsonData*> & v)  // NOLINT(runtime/explicit)
    : ordered_map<JsonString, JsonData*>(v) {}
};

class JsonNull : public JsonData {};

// Hack to make << operator work multi-threaded
thread_local int level = 0;

std::string getIndent(int l) {
    std::stringstream ss;
    for (int i = 0; i < l*4; i++) ss << " ";
    return ss.str();
}

std::ostream& operator<<(std::ostream &out, JsonData* json) {
    if (dynamic_cast<JsonObject*>(json)) {
        auto obj = dynamic_cast<ordered_map<JsonString, JsonData*>*>(json);
        out << "{";
        if (obj->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *obj)
                out << getIndent(level) << e.first << " : " << e.second << "," << std::endl;
            out << getIndent(--level);
        }
        out << "}";
    } else if (dynamic_cast<JsonVector*>(json)) {
        std::vector<JsonData*>* vec = dynamic_cast<std::vector<JsonData*>*>(json);
        out << "[";
        if (vec->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *vec) {
                out << getIndent(level) << e << "," << std::endl;
            }
            out << getIndent(--level);
        }
        out << "]";
    } else if (dynamic_cast<JsonString*>(json)) {
        JsonString* s = dynamic_cast<JsonString*>(json);
        out << "\"" << s->c_str() << "\"";

    } else if (dynamic_cast<JsonNumber*>(json)) {
        JsonNumber* num = dynamic_cast<JsonNumber*>(json);
        out << num->val;

    } else if (dynamic_cast<JsonBoolean*>(json)) {
        JsonBoolean *b = dynamic_cast<JsonBoolean*>(json);
        out << (b->val ? "true" : "false");
    } else if (dynamic_cast<JsonNull*>(json)) {
        out << "null";
    }
    return out;
}


std::istream& operator>>(std::istream &in, JsonData*& json) {
    while (in) {
        char ch;
        in >> ch;
        switch (ch) {
        case '{': {
            ordered_map<JsonString, JsonData*> obj;
            do {
                in >> std::ws >> ch;
                if (ch == '}')
                    break;
                in.unget();

                JsonData *key, *val;
                in >> key >> std::ws >> ch >> std::ws >> val;
                obj[*(dynamic_cast<std::string*>(key))] = val;

                in >> std::ws >> ch;
            } while (in && ch != '}');

            json = new JsonObject(obj);
            return in;
        }
        case '[': {
            std::vector<JsonData*> vec;
            do {
                in >> std::ws >> ch;
                if (ch == ']')
                    break;
                in.unget();

                JsonData *elem;
                in >> elem;
                vec.push_back(elem);

                in >> std::ws >> ch;
            } while (in && ch != ']');

            json = new JsonVector(vec);
            return in;
        }
        case '"': {
            std::string s;
            getline(in, s, '"');
            json = new JsonString(s);
            return in;
        }
        case '-': case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': case '8': case '9': {
            long long num;
            in.unget();
            in >> num;
            json = new JsonNumber(num);
            return in;
        }
        case 't': case 'T':
            in.ignore(3);
            json = new JsonBoolean(true);
            return in;
        case 'f': case 'F':
            in.ignore(4);
            json = new JsonBoolean(false);
            return in;
        case 'n': case 'N':
            in.ignore(3);
            json = new JsonNull();
            return in;
        }
    }
}

#endif /* IR_JSON_PARSER_H_ */
