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

#ifndef LIB_INDENT_H_
#define LIB_INDENT_H_

#include <iomanip>
#include <iostream>
#include <vector>

class indent_t {
    int indent;

 public:
    static int tabsz;
    indent_t() : indent(0) {}
    explicit indent_t(int i) : indent(i) {}
    indent_t &operator++() {
        ++indent;
        return *this;
    }
    indent_t &operator--() {
        --indent;
        return *this;
    }
    indent_t operator++(int) {
        indent_t rv = *this;
        ++indent;
        return rv;
    }
    indent_t operator--(int) {
        indent_t rv = *this;
        --indent;
        return rv;
    }
    friend std::ostream &operator<<(std::ostream &os, indent_t i);
    indent_t operator+(int v) {
        indent_t rv = *this;
        rv.indent += v;
        return rv;
    }
    indent_t operator-(int v) {
        indent_t rv = *this;
        rv.indent -= v;
        return rv;
    }
    indent_t &operator+=(int v) {
        indent += v;
        return *this;
    }
    indent_t &operator-=(int v) {
        indent -= v;
        return *this;
    }
    static indent_t &getindent(std::ostream &);
};

inline std::ostream &operator<<(std::ostream &os, indent_t i) {
    os << std::setw(i.indent * i.tabsz) << "";
    return os;
}

namespace IndentCtl {
inline std::ostream &endl(std::ostream &out) {
    return out << std::endl << indent_t::getindent(out);
}
inline std::ostream &indent(std::ostream &out) {
    ++indent_t::getindent(out);
    return out;
}
inline std::ostream &unindent(std::ostream &out) {
    --indent_t::getindent(out);
    return out;
}

class TempIndent {
    // an indent that can be added to any stream and unrolls when the object is destroyed
    std::vector<std::ostream *> streams;      // streams that have been indented
    TempIndent(const TempIndent &) = delete;  // not copyable

 public:
    TempIndent() = default;
    friend std::ostream &operator<<(std::ostream &out, TempIndent &ti) {
        ti.streams.push_back(&out);
        return out << indent;
    }
    friend std::ostream &operator<<(std::ostream &out, TempIndent &&ti) {
        ti.streams.push_back(&out);
        return out << indent;
    }
    const char *pop_back() {
        if (!streams.empty()) {
            *streams.back() << unindent;
            streams.pop_back();
        }
        return "";
    }
    const char *reset() {
        for (auto *out : streams) *out << unindent;
        streams.clear();
        return "";
    }
    ~TempIndent() {
        for (auto *out : streams) *out << unindent;
    }
};

}  // end namespace IndentCtl

#endif /* LIB_INDENT_H_ */
