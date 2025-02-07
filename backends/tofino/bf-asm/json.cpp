/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backends/tofino/bf-asm/json.h"

#include <iomanip>
#include <sstream>

#include "lib/hex.h"

namespace json {

static int digit_value(char ch) {
    if (ch >= 'a') return ch - 'a' + 10;
    if (ch >= 'A') return ch - 'A' + 10;
    if (ch >= '0' && ch <= '9') return ch - '0';
    return 999;
}

// true iff the string ends in an odd number of '\' characters
static bool odd_backslash(const std::string &s) {
    int cnt = 0;
    for (int i = s.size() - 1; i >= 0; --i) {
        if (s[i] != '\\') break;
        cnt++;
    }
    return (cnt & 1) == 1;
}

std::istream &operator>>(std::istream &in, std::unique_ptr<obj> &json) {
    while (in) {
        bool neg = false;
        char ch;
        int base = 10, digit;
        in >> ch;
        switch (ch) {
            case '-':
                neg = true;
                in >> ch;
                if (ch != '0') goto digit;
                /* fall through */
            case '0':
                base = 8;
                in >> ch;
                if (ch == 'x' || ch == 'X') {
                    base = 16;
                    in >> ch;
                } else if (ch == 'b') {
                    base = 2;
                    in >> ch;
                }
                /* fall through */
            digit:
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                int64_t l = 0;
                while (in && (digit = digit_value(ch)) < base) {
                    if ((INT64_MAX - digit) / base < l) {
                        std::cerr << "overflow detected" << std::endl;
                    }
                    l = l * base + digit;
                    in >> ch;
                }
                if (in) in.unget();
                if (neg) l = -l;
                json.reset(new number(l));
                return in;
            }
            case '"': {
                std::string s;
                getline(in, s, '"');
                while (odd_backslash(s)) {
                    std::string tmp;
                    getline(in, tmp, '"');
                    s += '\"';
                    s += tmp;
                }
                json.reset(new string(std::move(s)));
                return in;
            }
            case '[': {
                std::unique_ptr<vector> rv(new vector());
                in >> ch;
                if (ch != ']') {
                    in.unget();
                    do {
                        std::unique_ptr<obj> o;
                        in >> o >> ch;
                        rv->push_back(std::move(o));
                        if (ch != ',' && ch != ']') {
                            std::cerr << "missing ',' in vector (saw '" << ch << "')" << std::endl;
                            in.unget();
                        }
                    } while (in && ch != ']');
                }
                json = std::move(rv);
                return in;
            }
            case '{': {
                std::unique_ptr<map> rv(new map());
                in >> ch;
                if (ch != '}') {
                    in.unget();
                    do {
                        std::unique_ptr<obj> key, val;
                        in >> key >> ch;
                        if (ch == '}') {
                            std::cerr << "missing value in map" << std::endl;
                        } else {
                            if (ch != ':') {
                                std::cerr << "missing ':' in map (saw '" << ch << "')" << std::endl;
                                in.unget();
                            }
                            in >> val >> ch;
                        }
                        if (rv->count(key.get()))
                            std::cerr << "duplicate key in map" << std::endl;
                        else
                            (*rv)[std::move(key)] = std::move(val);
                        if (ch != ',' && ch != '}') {
                            std::cerr << "missing ',' in map (saw '" << ch << "')" << std::endl;
                            in.unget();
                        }
                    } while (in && ch != '}');
                }
                json = std::move(rv);
                return in;
            }
            default:
                if (isalpha(ch) || ch == '_') {
                    std::string s;
                    while (isalnum(ch) || ch == '_') {
                        s += ch;
                        if (!(in >> ch)) break;
                    }
                    in.unget();
                    if (s == "true")
                        json.reset(new True());
                    else if (s == "false")
                        json.reset(new False());
                    else if (s == "null")
                        json.reset();
                    else
                        json.reset(new string(std::move(s)));
                    return in;
                } else {
                    std::cerr << "unexpected character '" << ch << "' (0x" << hex(ch) << ")"
                              << std::endl;
                }
        }
    }
    return in;
}

void vector::print_on(std::ostream &out, int indent, int width, const char *pfx) const {
    int twidth = width;
    bool first = true;
    bool oneline = test_width(twidth);
    out << '[';
    indent += 2;
    for (auto &e : *this) {
        if (!first) out << ',';
        if (!oneline) out << '\n' << pfx << std::setw(indent);
        out << ' ' << std::setw(0);
        if (e)
            e->print_on(out, indent, width - 2, pfx);
        else
            out << "null";
        first = false;
    }
    indent -= 2;
    if (!first) out << (oneline ? ' ' : '\n');
    if (!oneline) out << std::setw(indent + 1);
    out << ']';
}

void map::print_on(std::ostream &out, int indent, int width, const char *pfx) const {
    int twidth = width;
    bool first = true;
    bool oneline = test_width(twidth);
    // std::cout << "*** width=" << width << "  twdith=" << twidth << std::endl;
    out << '{';
    indent += 2;
    for (auto &e : *this) {
        if (!first) out << ',';
        if (!oneline) out << '\n' << pfx << std::setw(indent);
        out << ' ' << std::setw(0);
        e.first->print_on(out, indent, width - 2, pfx);
        out << ": ";
        if (e.second)
            e.second->print_on(out, indent, width - 2, pfx);
        else
            out << "null";
        first = false;
    }
    indent -= 2;
    if (!first) out << (oneline ? ' ' : '\n');
    if (!oneline) out << std::setw(indent + 1);
    out << '}';
}

std::string obj::toString() const {
    std::stringstream buf;
    print_on(buf);
    return buf.str();
}

map &map::merge(const map &a) {
    for (auto &el : a) {
        if (!el.second) {
            erase(el.first);
        } else if (count(el.first)) {
            auto &exist = at(el.first);
            if (exist->is<map>() && el.second->is<map>()) {
                exist->to<map>().merge(el.second->to<map>());
            } else if (exist->is<vector>() && el.second->is<vector>()) {
                auto &vec = exist->to<vector>();
                for (auto &vel : el.second->to<vector>()) vec.push_back(vel->clone());
            } else {
                exist = el.second->clone();
            }
        } else {
            emplace(el.first->clone().release(), el.second->clone());
        }
    }
    return *this;
}

}  // namespace json

void dump(const json::obj &o) { std::cout << &o << std::endl; }
void dump(const json::obj *o) { std::cout << o << std::endl; }
