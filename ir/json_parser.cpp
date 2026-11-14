// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

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

void dump(const JsonData *json) { std::cout << json << std::endl; }

bool JsonData::strict = false;

static inline std::streamoff lastpos(std::istream &in) {
    std::streamoff pos = in.tellg();
    if (pos > 0) --pos;
    return pos;
}

static std::string token(std::istream &in) {
    std::string rv;
    char ch;
    while (in && (isalnum(static_cast<unsigned char>(ch = in.get())) || ch == '_')) rv += ch;
    if (in) in.unget();
    return rv;
}

// Validate JSON escapes before using the existing C escape decoder. JSON has
// fewer escape forms, and represents supplementary characters as surrogate pairs.
static std::string jsonStringEscapes(std::string_view raw, std::streamoff position) {
    std::string result;
    auto invalid = [&]() { throw JsonData::error("invalid JSON string escape", position); };
    auto hex4 = [&](size_t offset) {
        unsigned value = 0;
        if (offset + 4 > raw.size()) invalid();
        for (size_t i = offset; i < offset + 4; ++i) {
            char c = raw[i];
            unsigned digit = c >= '0' && c <= '9'   ? c - '0'
                             : c >= 'a' && c <= 'f' ? c - 'a' + 10
                             : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                                    : 16;
            if (digit == 16) invalid();
            value = value * 16 + digit;
        }
        return value;
    };
    for (size_t i = 0; i < raw.size(); ++i) {
        if (static_cast<unsigned char>(raw[i]) < 0x20) invalid();
        if (raw[i] != '\\') {
            result += raw[i];
            continue;
        }
        if (++i == raw.size()) invalid();
        if (raw[i] == '/') {
            result += '/';
        } else if (raw[i] == 'u') {
            auto codepoint = hex4(i + 1);
            i += 4;
            if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
                if (i + 6 >= raw.size() || raw.substr(i + 1, 2) != "\\u") invalid();
                auto low = hex4(i + 3);
                if (low < 0xdc00 || low > 0xdfff) invalid();
                codepoint = 0x10000 + (codepoint - 0xd800) * 0x400 + low - 0xdc00;
                i += 6;
            } else if (codepoint >= 0xdc00 && codepoint <= 0xdfff) {
                invalid();
            }
            // CUnescape accepts a scalar through its eight-digit Unicode form.
            constexpr char hex[] = "0123456789abcdef";
            result += "\\U";
            for (int shift = 28; shift >= 0; shift -= 4) result += hex[(codepoint >> shift) & 15];
        } else {
            if (std::string_view("\"\\bfnrt").find(raw[i]) == std::string_view::npos) invalid();
            result += '\\';
            result += raw[i];
        }
    }
    return result;
}

std::istream &parseJson(std::istream &in, std::unique_ptr<JsonData> &json, bool strict) {
    while (in) {
        char ch = 0;
        in >> std::ws >> ch;
        std::streamoff start = lastpos(in);
        switch (ch) {
            case '{': {
                string_map<std::unique_ptr<JsonData>> obj;
                do {
                    in >> std::ws >> ch;
                    if (ch == '}') {
                        if (strict && !obj.empty())
                            throw JsonData::error("extra ',' at end of object", lastpos(in));
                        break;
                    }
                    in.unget();

                    std::unique_ptr<JsonData> key, val;
                    parseJson(in, key, strict) >> std::ws >> ch;
                    if (strict && (!in || ch != ':'))
                        throw JsonData::error("missing ':' in object", lastpos(in));
                    parseJson(in, val, strict);
                    if (strict && (!key || !key->is<JsonString>() || !val))
                        throw JsonData::error("invalid object member", lastpos(in));
                    obj[key->as<JsonString>()] = std::move(val);

                    in >> std::ws >> ch;
                    if (strict && (!in || (ch != ',' && ch != '}')))
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
                        if (strict && !vec.empty())
                            throw JsonData::error("extra ',' at end of vector", lastpos(in));
                        break;
                    }
                    in.unget();

                    std::unique_ptr<JsonData> elem;
                    parseJson(in, elem, strict);
                    vec.emplace_back(std::move(elem));

                    in >> std::ws >> ch;
                    if (strict && (!in || (ch != ',' && ch != ']')))
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
                if (strict && !in) throw JsonData::error("unterminated string", start);
                if (strict) s = jsonStringEscapes(s, start);
                if (!absl::CUnescape(s, &s) && strict)
                    throw JsonData::error("invalid string escape", start);
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
                if (strict) {
                    num += ch;
                    while (in && std::isdigit(in.peek())) num += static_cast<char>(in.get());
                    auto digits = std::string_view(num);
                    if (digits.starts_with('-')) digits.remove_prefix(1);
                    if (digits.empty() || (digits.size() > 1 && digits.front() == '0'))
                        throw JsonData::error("invalid JSON integer", start);
                } else {
                    do {
                        num += ch;
                        in >> ch;
                    } while (isdigit(static_cast<unsigned char>(ch)));
                    in.unget();
                }
                json = std::make_unique<JsonNumber>(big_int(num));
                json->start = start;
                json->finish = lastpos(in);
                return in;
            }
            case 't':
            case 'T':
                if (strict) {
                    auto tok = token(in);
                    if (ch != 't' || tok != "rue") {
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
                if (strict) {
                    auto tok = token(in);
                    if (ch != 'f' || tok != "alse") {
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
                if (strict) {
                    auto tok = token(in);
                    if (ch != 'n' || tok != "ull") {
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
                if (strict) {
                    if (isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
                        auto tok = token(in);
                        tok = "Unexpected token " + (ch + tok);
                        throw JsonData::error(tok, start);
                    } else if (!isspace(static_cast<unsigned char>(ch))) {
                        std::string msg = "Unexpected character '";
                        msg += ch;
                        msg += '\'';
                        throw JsonData::error(msg, start);
                    }
                }
                if (in && !isspace(static_cast<unsigned char>(ch))) in.unget();
                return in;
        }
    }
    return in;
}

std::istream &operator>>(std::istream &in, std::unique_ptr<JsonData> &json) {
    return parseJson(in, json, JsonData::strict);
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

std::string JsonData::LocationInfo::desc(const JsonData::error &e) {
    if (e.data) return desc(*e.data);
    return desc(e.loc);
}

}  // namespace P4
