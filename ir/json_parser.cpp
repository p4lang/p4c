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

#include "absl/strings/escaping.h"

namespace P4 {

// Hack to make << operator work multi-threaded
static thread_local int level = 0;

std::string getIndent(int l) {
    std::stringstream ss;
    for (int i = 0; i < l * 4; i++) ss << " ";
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const JsonData *json) {
    if (auto *obj = json->to<JsonObject>()) {
        out << "{";
        if (obj->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *obj)
                out << getIndent(level) << e.first << " : " << e.second.get() << "," << std::endl;
            out << getIndent(--level);
        }
        out << "}";
    } else if (auto *vec = json->to<JsonVector>()) {
        out << "[";
        if (vec->size() > 0) {
            level++;
            out << std::endl;
            for (auto &e : *vec) {
                out << getIndent(level) << e.get() << "," << std::endl;
            }
            out << getIndent(--level);
        }
        out << "]";
    } else if (auto *s = json->to<JsonString>()) {
        out << "\"" << cstring(*s).escapeJson() << "\"";

    } else if (auto *num = json->to<JsonNumber>()) {
        out << num->val;

    } else if (auto *b = json->to<JsonBoolean>()) {
        out << (b->val ? "true" : "false");
    } else if (json->is<JsonNull>()) {
        out << "null";
    }
    return out;
}

std::istream &operator>>(std::istream &in, std::unique_ptr<JsonData> &json) {
    while (in) {
        char ch;
        in >> ch;
        switch (ch) {
            case '{': {
                string_map<std::unique_ptr<JsonData>> obj;
                do {
                    in >> std::ws >> ch;
                    if (ch == '}') break;
                    in.unget();

                    std::unique_ptr<JsonData> key, val;
                    in >> key >> std::ws >> ch >> std::ws >> val;
                    obj[key->as<JsonString>()] = std::move(val);

                    in >> std::ws >> ch;
                } while (in && ch != '}');

                json = std::make_unique<JsonObject>(std::move(obj));
                return in;
            }
            case '[': {
                std::vector<std::unique_ptr<JsonData>> vec;
                do {
                    in >> std::ws >> ch;
                    if (ch == ']') break;
                    in.unget();

                    std::unique_ptr<JsonData> elem;
                    in >> elem;
                    vec.emplace_back(std::move(elem));

                    in >> std::ws >> ch;
                } while (in && ch != ']');

                json = std::make_unique<JsonVector>(std::move(vec));
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
                absl::CUnescape(s, &s);
                json = std::make_unique<JsonString>(s);
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
                json = std::make_unique<JsonNumber>(big_int(num));
                return in;
            }
            case 't':
            case 'T':
                in.ignore(3);
                json = std::make_unique<JsonBoolean>(true);
                return in;
            case 'f':
            case 'F':
                in.ignore(4);
                json = std::make_unique<JsonBoolean>(false);
                return in;
            case 'n':
            case 'N':
                in.ignore(3);
                json = std::make_unique<JsonNull>();
                return in;
            default:
                return in;
        }
    }
    return in;
}

}  // namespace P4
