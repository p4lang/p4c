/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_INDENT_H_
#define LIB_INDENT_H_

#include <iomanip>
#include <iostream>
#include <vector>

namespace P4 {

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
    std::vector<std::ostream *> streams;  // streams that have been indented

 public:
    TempIndent(const TempIndent &) = delete;  // not copyable
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

}  // namespace IndentCtl
}  // namespace P4

#endif /* LIB_INDENT_H_ */
