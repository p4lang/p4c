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

#ifndef P4C_LIB_CSTRING_H_
#define P4C_LIB_CSTRING_H_

#include <string.h>

#include <iostream>
#include <string>
#include <sstream>

// cstring is a zero-terminated, constant (immutable) string
class cstring {
    const char *str;

 public:
    // cstring() = default;
    cstring() : str(0) {}
    cstring(const cstring &) = default;
    cstring(cstring &&) = default;
    cstring &operator=(const cstring &) = default;
    cstring &operator=(cstring &&) = default;
    cstring &operator=(const char *);
    cstring(const std::stringstream&);                      // NOLINT(runtime/explicit)
    cstring(const char *s) { *this = s; }                   // NOLINT(runtime/explicit)
    cstring(const std::string &a) { *this = a.c_str(); }    // NOLINT(runtime/explicit)
    const char *c_str() const { return str; }
    const char *find(int c) const { return str ? strchr(str, c) : nullptr; }
    const char *findlast(int c) const { return str ? strrchr(str, c) : str; }
    operator const char *() const { return str; }
    size_t size() const { return str ? strlen(str) : 0; }
    bool isNull() const { return str == nullptr; }
    bool isNullOrEmpty() const { return str == nullptr ? true : str[0] == 0; }
    bool operator==(const cstring &a) const { return str == a.str; }
    bool operator==(const char *a) const { return str ? a && !strcmp(str, a) : !a; }
    bool operator!=(const cstring &a) const { return str != a.str; }
    bool operator!=(const char *a) const { return str ? !a || !!strcmp(str, a) : !!a; }
    bool operator<(const cstring &a) const { return *this < a.str; }
    bool operator<(const char *a) const { return str ? a && strcmp(str, a) < 0 : !!a; }
    bool operator<=(const cstring &a) const { return *this <= a.str; }
    bool operator<=(const char *a) const { return str ? a && strcmp(str, a) <= 0 : true; }
    bool operator>(const cstring &a) const { return *this > a.str; }
    bool operator>(const char *a) const { return str ? !a || strcmp(str, a) > 0 : false; }
    bool operator>=(const cstring &a) const { return *this >= a.str; }
    bool operator>=(const char *a) const { return str ? !a || strcmp(str, a) >= 0 : !a; }

    bool operator==(const std::string &a) const { return *this == a.c_str(); }
    bool operator!=(const std::string &a) const { return *this != a.c_str(); }
    bool operator<(const std::string &a) const { return *this < a.c_str(); }
    bool operator<=(const std::string &a) const { return *this <= a.c_str(); }
    bool operator>(const std::string &a) const { return *this > a.c_str(); }
    bool operator>=(const std::string &a) const { return *this >= a.c_str(); }

    cstring operator+=(cstring a);
    cstring operator+=(const char *a);
    cstring operator+=(std::string a);
    cstring operator+=(char a);
    bool startsWith(const cstring& prefix) const;
    bool endsWith(const cstring& suffix) const;
    template<typename T>
    static cstring to_cstring(const T &t) {
        std::stringstream ss;
        ss << t;
        return cstring(ss.str()); }
    template<typename Iterator>
    static cstring join(Iterator begin, Iterator end, const char *delim = ", ") {
        std::stringstream ss;
        for (auto current = begin; current != end; ++current) {
            if (begin != current) ss << delim;
            ss << *current; }
        return cstring(ss.str()); }
    static cstring newline;
    static cstring empty;
    template<class T> static cstring make_unique(const T &inuse, cstring base, char sep = '.');
    cstring before(const char* at) const;
    cstring substr(size_t start) const
    { return (start >= size()) ? "" : substr(start, size() - start); }
    cstring substr(size_t start, size_t length) const;
    cstring replace(char find, char replace) const;
    static size_t cache_size(size_t &);
};

inline bool operator==(const char *a, cstring b) { return b == a; }
inline bool operator!=(const char *a, cstring b) { return b != a; }

inline std::string operator+(cstring a, cstring b) {
    std::string rv(a); rv += b; return rv; }
inline std::string operator+(cstring a, const char *b) {
    std::string rv(a); rv += b; return rv; }
inline std::string operator+(cstring a, const std::string &b) {
    std::string rv(a); rv += b; return rv; }
inline std::string operator+(cstring a, char b) {
    std::string rv(a); rv += b; return rv; }
inline std::string operator+(const char *a, cstring b) {
    std::string rv(a); rv += b; return rv; }
inline std::string operator+(std::string a, cstring b) { a += b; return a; }
inline std::string operator+(char a, cstring b) {
    std::string rv(1, a); rv += b; return rv; }

inline cstring cstring::operator+=(cstring a) { *this = *this + a; return *this; }
inline cstring cstring::operator+=(const char *a) { *this = *this + a; return *this; }
inline cstring cstring::operator+=(std::string a) { *this = *this + a; return *this; }
inline cstring cstring::operator+=(char a) { *this = *this + a; return *this; }

inline std::string operator+=(std::string a, cstring b) {
    a.append(b.c_str());
    return a; }

template<class T> cstring cstring::make_unique(const T &inuse, cstring base, char sep) {
    char suffix[8];
    cstring rv = base;
    int counter = 0;
    while (inuse.count(rv)) {
        snprintf(suffix, sizeof(suffix)/sizeof(suffix[0]), "%c%d", sep, counter++);
        rv = base + suffix; }
    return rv; }

inline std::ostream &operator<<(std::ostream &out, cstring s) {
    return out << (s ? s.c_str() : "<null>"); }

namespace std {
template<> struct hash<cstring> {
    std::size_t operator()(const cstring& c) const {
        // This implementation is good for cstring, since the strings are internalized
        return hash<const void *>()(c.c_str());
    }
};
}  // namespace std

#endif /* P4C_LIB_CSTRING_H_ */
