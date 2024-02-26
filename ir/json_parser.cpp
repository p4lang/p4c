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

#include "ir/json_parser.h"

#include <cctype>
#include <iostream>
#include <list>
#include <utility>

int JsonObject::get_id() const {
    auto it = find("Node_ID");
    if (it == end()) return -1;

    return it->second->as<JsonNumber>();
}

std::string JsonObject::get_type() const {
    auto it = find("Node_Type");
    if (it == end()) return "";

    return it->second->as<JsonString>();
}

std::string JsonObject::get_filename() const {
    auto it = find("filename");

    if (it == end()) return "";

    return it->second->as<JsonString>();
}

std::string JsonObject::get_sourceFragment() const {
    auto it = find("source_fragment");

    if (it == end()) return "";

    return it->second->as<JsonString>();
}

int JsonObject::get_line() const {
    auto it = find("line");

    if (it == end()) return -1;

    return it->second->as<JsonNumber>();
}

int JsonObject::get_column() const {
    auto it = find("column");

    if (it == end()) return -1;

    return it->second->as<JsonNumber>();
}

JsonObject JsonObject::get_sourceJson() const {
    auto it = find("Source_Info");

    if (it == end()) {
        JsonObject obj;
        obj.setSrcInfo(false);
        return obj;
    }

    return it->second->as<JsonObject>();
}

// Hack to make << operator work multi-threaded
static thread_local int level = 0;

std::string getIndent(int l) {
    std::stringstream ss;
    for (int i = 0; i < l * 4; i++) ss << " ";
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, JsonData *json) {
    if (auto *obj = json->to<JsonObject>()) {
        out << "{";
        if (obj->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *obj)
                out << getIndent(level) << e.first << " : " << e.second << "," << std::endl;
            out << getIndent(--level);
        }
        out << "}";
    } else if (auto *vec = json->to<JsonVector>()) {
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
    } else if (auto *s = json->to<JsonString>()) {
        out << "\"" << s->c_str() << "\"";

    } else if (auto *num = json->to<JsonNumber>()) {
        out << num->val;

    } else if (auto *b = json->to<JsonBoolean>()) {
        out << (b->val ? "true" : "false");
    } else if (json->is<JsonNull>()) {
        out << "null";
    }
    return out;
}

std::istream &operator>>(std::istream &in, JsonData *&json) {
    while (in) {
        char ch;
        in >> ch;
        switch (ch) {
            case '{': {
                ordered_map<std::string, JsonData *> obj;
                do {
                    in >> std::ws >> ch;
                    if (ch == '}') break;
                    in.unget();

                    JsonData *key, *val;
                    in >> key >> std::ws >> ch >> std::ws >> val;
                    obj[key->as<JsonString>()] = val;

                    in >> std::ws >> ch;
                } while (in && ch != '}');

                json = new JsonObject(obj);
                return in;
            }
            case '[': {
                std::vector<JsonData *> vec;
                do {
                    in >> std::ws >> ch;
                    if (ch == ']') break;
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
                while (!s.empty() && s.back() == '\\') {
                    int bscount = 0;  // odd number of '\' chars mean the quote is escaped
                    for (auto t = s.rbegin(); t != s.rend() && *t == '\\'; ++t) bscount++;
                    if ((bscount & 1) == 0) break;
                    s += '"';
                    std::string more;
                    getline(in, more, '"');
                    s += more;
                }
                json = new JsonString(s);
                return in;
            }
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                // operator>>(istream, big_int) is broken and throws exceptions if the
                // number is not followed by whitespace, so we need to manually extract all
                // the digits into a buffer and convert that to big_int
                std::string num;
                do {
                    num += ch;
                    in >> ch;
                } while (isdigit(ch));
                in.unget();
                json = new JsonNumber(big_int(num));
                return in;
            }
            case 't':
            case 'T':
                in.ignore(3);
                json = new JsonBoolean(true);
                return in;
            case 'f':
            case 'F':
                in.ignore(4);
                json = new JsonBoolean(false);
                return in;
            case 'n':
            case 'N':
                in.ignore(3);
                json = new JsonNull();
                return in;
            default:
                return in;
        }
    }
    return in;
}
