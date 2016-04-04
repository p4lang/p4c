#ifndef P4C_LIB_INDENT_H_
#define P4C_LIB_INDENT_H_

#include <iostream>
#include <iomanip>

class indent_t {
    int         indent;
 public:
    static int  tabsz;
    indent_t() : indent(0) {}
    explicit indent_t(int i) : indent(i) {}
    indent_t &operator++() { ++indent; return *this; }
    indent_t &operator--() { --indent; return *this; }
    indent_t operator++(int) { indent_t rv = *this; ++indent; return rv; }
    indent_t operator--(int) { indent_t rv = *this; --indent; return rv; }
    friend std::ostream &operator<<(std::ostream &os, indent_t i);
    indent_t operator+(int v) { indent_t rv = *this; rv.indent += v; return rv; }
    indent_t operator-(int v) { indent_t rv = *this; rv.indent -= v; return rv; }
    indent_t &operator+=(int v) { indent += v; return *this; }
    indent_t &operator-=(int v) { indent -= v; return *this; }
    static indent_t &getindent(std::ostream &);
};

inline std::ostream &operator<<(std::ostream &os, indent_t i) {
    os << std::setw(i.indent * i.tabsz) << "";
    return os;
}

namespace IndentCtl {
inline std::ostream &endl(std::ostream &out) {
    return out << std::endl << indent_t::getindent(out); }
inline std::ostream &indent(std::ostream &out) { ++indent_t::getindent(out); return out; }
inline std::ostream &unindent(std::ostream &out) { --indent_t::getindent(out); return out; }
}  // end namespace IndentCtl

#endif /* P4C_LIB_INDENT_H_ */
