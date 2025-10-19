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

bool JsonData::strict = false;

static inline std::streamoff lastpos(std::istream &in) {
    std::streamoff pos = in.tellg();
    if (pos > 0) --pos;
    return pos;
}

static std::string token(std::istream &in) {
    std::string rv;
    char ch;
    while (in && (isalnum((ch = in.get())) || ch == '_')) rv += ch;
    if (in) in.unget();
    return rv;
}

std::istream &operator>>(std::istream &in, std::unique_ptr<JsonData> &json) {
    while (in) {
        char ch;
        in >> std::ws >> ch;
        std::streamoff start = lastpos(in);
        switch (ch) {
            case '{': {
                string_map<std::unique_ptr<JsonData>> obj;
                do {
                    in >> std::ws >> ch;
                    if (ch == '}') {
                        if (JsonData::strict && !obj.empty())
                            throw JsonData::error("extra ',' at end of object", lastpos(in));
                        break;
                    }
                    in.unget();

                    std::unique_ptr<JsonData> key, val;
                    in >> key >> std::ws >> ch;
                    if (JsonData::strict && (!in || ch != ':'))
                        throw JsonData::error("missing ':' in object", lastpos(in));
                    in >> val;
                    obj[key->as<JsonString>()] = std::move(val);

                    in >> std::ws >> ch;
                    if (JsonData::strict && (!in || (ch != ',' && ch != '}')))
                        throw JsonData::error("missing ',' in object", lastpos(in));
                } while (in && ch != '}');

                json = std::make_unique<JsonObject>(std::move(obj));
                json->start = start;
                json->finish = lastpos(in);
                return in;
            }
            case '[': {
                std::vector<std::unique_ptr<JsonData>> vec;
                do {
                    in >> std::ws >> ch;
                    if (ch == ']') {
                        if (JsonData::strict && !vec.empty())
                            throw JsonData::error("extra ',' at end of vector", lastpos(in));
                        break;
                    }
                    in.unget();

                    std::unique_ptr<JsonData> elem;
                    in >> elem;
                    vec.emplace_back(std::move(elem));

                    in >> std::ws >> ch;
                    if (JsonData::strict && (!in || (ch != ',' && ch != ']')))
                        throw JsonData::error("missing ',' in vector", lastpos(in));
                } while (in && ch != ']');

                json = std::make_unique<JsonVector>(std::move(vec));
                json->start = start;
                json->finish = lastpos(in);
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
                json->start = start;
                json->finish = lastpos(in);
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
                json->start = start;
                json->finish = lastpos(in);
                return in;
            }
            case 't':
            case 'T':
                if (JsonData::strict) {
                    auto tok = token(in);
                    if (tok != "rue") {
                        tok = "Unexpected token " + (ch + tok);
                        throw JsonData::error(tok, start);
                    }
                } else {
                    in.ignore(3);
                }
                json = std::make_unique<JsonBoolean>(true);
                json->start = start;
                json->finish = lastpos(in);
                return in;
            case 'f':
            case 'F':
                if (JsonData::strict) {
                    auto tok = token(in);
                    if (tok != "alse") {
                        tok = "Unexpected token " + (ch + tok);
                        throw JsonData::error(tok, start);
                    }
                } else {
                    in.ignore(4);
                }
                json = std::make_unique<JsonBoolean>(false);
                json->start = start;
                json->finish = lastpos(in);
                return in;
            case 'n':
            case 'N':
                if (JsonData::strict) {
                    auto tok = token(in);
                    if (tok != "ull") {
                        tok = "Unexpected token " + (ch + tok);
                        throw JsonData::error(tok, start);
                    }
                } else {
                    in.ignore(3);
                }
                json = std::make_unique<JsonNull>();
                json->start = start;
                json->finish = lastpos(in);
                return in;
            default:
                if (JsonData::strict) {
                    if (isalpha(ch) || ch == '_') {
                        auto tok = token(in);
                        tok = "Unexpected token " + (ch + tok);
                        throw JsonData::error(tok, start);
                    } else if (!isspace(ch)) {
                        std::string msg = "Unexpected character '";
                        msg += ch;
                        msg += '\'';
                        throw JsonData::error(msg, start);
                    }
                }
                if (in && !isspace(ch)) in.unget();
                return in;
        }
    }
    return in;
}

std::pair<int, int> JsonData::LocationInfo::loc(std::streamoff l) {
    if (l < 0) return std::make_pair(-1, -1);
    auto it = line.upper_bound(l);
    --it;
    if (l > scanned) {
        in.clear();
        auto current = in.tellg();
        in.seekg(scanned);
        while (in && in.tellg() <= l) {
            if (in.get() == '\n') it = line.emplace_hint(line.end(), in.tellg(), it->second + 1);
        }
        scanned = in.tellg();
        in.seekg(current);
    }
    return std::make_pair(it->second, l - it->first + 1);
}

std::string JsonData::LocationInfo::desc(std::streamoff l) {
    if (l < 0) return "";
    auto lc = loc(l);
    return name + " line " + std::to_string(lc.first) + " col " + std::to_string(lc.second) + ":";
}

std::string JsonData::LocationInfo::desc(const JsonData &d) {
    if (d.finish < 0 || d.finish == d.start) return desc(d.start);
    if (d.start < 0) return desc(d.finish);
    auto s = loc(d.start), e = loc(d.finish);
    if (s.first != e.first)
        return name + " lines " + std::to_string(s.first) + " to " + std::to_string(e.first) + ":";
    return name + " line " + std::to_string(s.first) + " cols " + std::to_string(s.second) +
           " to " + std::to_string(e.second) + ":";
}

}  // namespace P4
