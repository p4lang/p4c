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

#include <cstring>
#include <cstddef>

#include <functional>
#include <iostream>
#include <string>
#include <sstream>

/**
 * A cstring is a reference to a zero-terminated, immutable, interned string.
 * The cstring object *itself* is not immutable; you can reassign it as
 * required, and it provides a mutable interface that copies the underlying
 * immutable string as needed.
 *
 * Compared to std::string, these are the benefits that cstring provides:
 *   - Copying and assignment are cheap. Since cstring only deals with immutable
 *     strings, these operations only involve pointer assignment.
 *   - Comparing cstrings for equality is cheap; interning makes it possible to
 *     test for equality using a simple pointer comparison.
 *   - The immutability of the underlying strings means that it's always safe to
 *     change a cstring, even if there are other references to it elsewhere.
 *   - The API offers a number of handy helper methods that aren't available on
 *     std::string.
 *
 * On the other hand, these are the disadvantages of using cstring:
 *   - Because cstring deals with immutable strings, any modification requires
 *     that the complete string be copied.
 *   - Interning has an initial cost: converting a const char*, a
 *     std::string, or a std::stringstream to a cstring requires copying it.
 *     Currently, this happens every time you perform the conversion, whether
 *     the string is already interned or not, unless the source is an
 *     std::string.
 *   - Interned strings can never be freed, so they'll stick around for the
 *     lifetime of the program.
 *   - The string interning cstring performs is currently not threadsafe, so you
 *     can't safely use cstrings off the main thread.
 *
 * Given these tradeoffs, the general rule of thumb to follow is that you should
 * try to convert strings to cstrings early and keep them in that form. That
 * way, you benefit from cheaper copying, assignment, and equality testing in as
 * many places as possible, and you avoid paying the cost of repeated
 * conversion.
 *
 * However, when you're building or mutating a string, you should use
 * std::string. Convert to a cstring only when the string is in its final form.
 * This ensures that you don't pay the time and space cost of interning every
 * intermediate version of the string.
 *
 * Note that cstring has implicit conversions to and from other string types.
 * This is convenient, but in performance-sensitive code it's good to be aware
 * that mixing the two types of strings can trigger a lot of implicit copies.
 */
class cstring {
    const char *str = nullptr;

 public:
    cstring() = default;
    // TODO (DanilLutsenko): Enable when initialization with 0 will be eliminated
    // cstring(std::nullptr_t) {} // NOLINT(runtime/explicit)

    // Copy and assignment from other kinds of strings

    // Owner of string is someone else, but we know size of string.
    // Do not use if possible, this is linear time operation if string
    // not exists in table, because the underlying string must be copied.
    cstring(const char *string, std::size_t length) {  // NOLINT(runtime/explicit)
        if (string != nullptr) {
            construct_from_shared(string, length);
        }
    }

    // Owner of string is someone else, we do not know size of string.
    // Do not use if possible, this is linear time operation if string
    // not exists in table, because the underlying string must be copied.
    cstring(const char *string) {  // NOLINT(runtime/explicit)
        if (string != nullptr) {
            construct_from_shared(string, std::strlen(string));
        }
    }

    // construct cstring from std::string. Do not use if possible, this is linear
    // time operation if string not exists in table, because the underlying string must be copied.
    cstring(const std::string &string) {  // NOLINT(runtime/explicit)
        construct_from_shared(string.data(), string.length());
    }

    // TODO (DanilLutsenko): Make special case for r-value std::string?

    // Just helper function, for lazies, who do not like to write .str()
    // Do not use it, implicit std::string construction with implicit overhead
    // TODO (DanilLutsenko): Remove it?
    cstring(const std::stringstream& stream)  // NOLINT(runtime/explicit)
        : cstring(stream.str()) {
    }

    // TODO (DanilLutsenko): Construct from StringRef?

    // String was created outside and cstring is unique owner of it.
    // cstring will control lifetime of passed object
    static cstring own(const char *string, std::size_t length) {
        if (string == nullptr) {
            return{};
        }

        cstring result;
        result.construct_from_unique(string, length);
        return result;
    }

    // construct cstring wrapper for literal
    template<typename T, std::size_t N,
        typename = typename std::enable_if<std::is_same<T, const char>::value>::type>
    static cstring literal(T (&string)[N]) {  // NOLINT(runtime/explicit)
        cstring result;
        result.construct_from_literal(string, N - 1  /* String length without null terminator */);
        return result;
    }

 private:
    // passed string is shared, we not unique owners
    void construct_from_shared(const char *string, std::size_t length);

    // we are unique owners of passed string
    void construct_from_unique(const char *string, std::size_t length);

    // string is literal
    void construct_from_literal(const char *string, std::size_t length);

 public:
    /// @return a version of the string where all necessary characters
    /// are properly escaped to make this into a json string (without
    /// the enclosing quotes).
    cstring escapeJson() const;

    template <typename Iter> cstring(Iter begin, Iter end) {
        *this = std::string(begin, end);
    }

    char get(unsigned index) const { return (index < size()) ? str[index] : 0; }
    const char *c_str() const { return str; }
    operator const char *() const { return str; }

    // Size tests. Constant time except for size(), which is linear time.
    size_t size() const {
        // TODO (DanilLutsenko): We store size of string in table on object construction,
        // compiler cannot optimize strlen if str not points to string literal.
        // Probably better fetch size from table or store it to cstring on construction.
        return str ? strlen(str) : 0;
    }
    bool isNull() const { return str == nullptr; }
    bool isNullOrEmpty() const { return str == nullptr ? true : str[0] == 0; }

    // iterate over characters
    const char *begin() const { return str; }
    const char *end() const { return str ? str + strlen(str) : str; }

    // Search for characters. Linear time.
    const char *find(int c) const { return str ? strchr(str, c) : nullptr; }
    const char *findlast(int c) const { return str ? strrchr(str, c) : str; }

    // Search for substring
    const char *find(const char *s) const { return str ? strstr(str, s) : nullptr; }

    // Equality tests with other cstrings. Constant time.
    bool operator==(cstring a) const { return str == a.str; }
    bool operator!=(cstring a) const { return str != a.str; }

    // Other comparisons and tests. Linear time.
    bool operator==(const char *a) const { return str ? a && !strcmp(str, a) : !a; }
    bool operator!=(const char *a) const { return str ? !a || !!strcmp(str, a) : !!a; }
    bool operator<(cstring a) const { return *this < a.str; }
    bool operator<(const char *a) const { return str ? a && strcmp(str, a) < 0 : !!a; }
    bool operator<=(cstring a) const { return *this <= a.str; }
    bool operator<=(const char *a) const { return str ? a && strcmp(str, a) <= 0 : true; }
    bool operator>(cstring a) const { return *this > a.str; }
    bool operator>(const char *a) const { return str ? !a || strcmp(str, a) > 0 : false; }
    bool operator>=(cstring a) const { return *this >= a.str; }
    bool operator>=(const char *a) const { return str ? !a || strcmp(str, a) >= 0 : !a; }

    bool operator==(const std::string &a) const { return *this == a.c_str(); }
    bool operator!=(const std::string &a) const { return *this != a.c_str(); }
    bool operator<(const std::string &a) const { return *this < a.c_str(); }
    bool operator<=(const std::string &a) const { return *this <= a.c_str(); }
    bool operator>(const std::string &a) const { return *this > a.c_str(); }
    bool operator>=(const std::string &a) const { return *this >= a.c_str(); }

    bool startsWith(const cstring& prefix) const;
    bool endsWith(const cstring& suffix) const;

    // FIXME (DanilLutsenko): We really need mutations for immutable string?
    // Probably better do transformation in std::string-like containter and
    // then place result to cstring if needed.

    // Mutation operations. These are linear time and always trigger a copy,
    // since the underlying string is immutable. (Note that this is true even
    // for substr(); cstrings are always null-terminated, so a copy is
    // required.)
    cstring operator+=(cstring a);
    cstring operator+=(const char *a);
    cstring operator+=(std::string a);
    cstring operator+=(char a);

    cstring before(const char* at) const;
    cstring substr(size_t start) const
    { return (start >= size()) ? "" : substr(start, size() - start); }
    cstring substr(size_t start, size_t length) const;
    cstring replace(char find, char replace) const;
    cstring replace(cstring find, cstring replace) const;
    cstring exceptLast(size_t count) { return substr(0, size() - count); }

    // trim leading and trailing whitespace (or other)
    cstring trim(const char *ws = " \t\r\n") const {
        if (!str) return *this;
        const char *start = str + strspn(str, ws);
        size_t len = strlen(start);
        while (len > 0 && strchr(ws, start[len-1])) --len;
        return cstring(start, len); }

    // Useful singletons.
    static cstring newline;
    static cstring empty;

    // Static factory functions.
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
    template<class T> static cstring make_unique(const T &inuse, cstring base, char sep = '.');

    /// @return the total size in bytes of all interned strings. @count is set
    /// to the total number of interned strings.
    static size_t cache_size(size_t &count);
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

inline std::string& operator+=(std::string& a, cstring b) {
    a.append(b.c_str());
    return a; }

template<class T> cstring cstring::make_unique(const T &inuse, cstring base, char sep) {
    char suffix[12];
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
