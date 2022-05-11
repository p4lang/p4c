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

#include "cstring.h"

#include <algorithm>
#include <ios>
#include <string>
#include <unordered_set>

#include "hash.h"

namespace {
enum class table_entry_flags {
    none,
    no_need_copy = 1 << 0,
    require_destruction = 1 << 1,
    inplace = 1 << 2,
};

inline table_entry_flags operator &(table_entry_flags l, table_entry_flags r) {
    return static_cast<table_entry_flags>(static_cast<int>(l) & static_cast<int>(r));
}

inline table_entry_flags operator |(table_entry_flags l, table_entry_flags r) {
    return static_cast<table_entry_flags>(static_cast<int>(l) | static_cast<int>(r));
}

// cache entry, ordered by string length
class table_entry {
    std::size_t m_length = 0;
    table_entry_flags m_flags = table_entry_flags::none;

    union {
        const char *m_string;
        char m_inplace_string[sizeof(const char *)];
    };

 public:
    // entry ctor, makes copy of passed string
    table_entry(const char *string, std::size_t length, table_entry_flags flags)
        : m_length(length) {
        if ((flags & table_entry_flags::no_need_copy) == table_entry_flags::no_need_copy) {
            // No need to copy object, it's view of string, string literal or string allocated
            // on heap and wrapped with cstring.
            // Inherit require_destruction flag here, because string can be allready
            // allocated on heap and wrapped with cstring object, so cstring owns the string now

            m_flags = flags & table_entry_flags::require_destruction;
            m_string = string;
            return;
        }

        if (length < sizeof(const char *)) {
            // String with length less than size of pointer store directly
            // in pointer, that hint allows reduce stack fragmentation.
            // We can make such optimization because std::unordered_set never
            // moves objects in memory on new element insert
            std::memcpy(m_inplace_string, string, length);
            m_inplace_string[length] = '\0';
            m_flags = table_entry_flags::inplace;
        } else {
            // Make copy of string elseware
            auto copy = new char[length + 1];
            std::memcpy(copy, string, length);
            copy[length] = '\0';
            m_string = copy;

            // destruction required, because we own copy of string
            m_flags = table_entry_flags::require_destruction;
        }
    }

    // table_entry moveable only
    table_entry(const table_entry &) = delete;

    table_entry(table_entry &&other) : m_length(other.m_length), m_flags(other.m_flags) {
        // this object for internal usage only, length will never be accessed
        // if object was moved, so do not zero other.m_length here

        if (is_inplace()) {
            std::memcpy(m_inplace_string, other.m_inplace_string, sizeof(m_inplace_string));
        } else {
            m_string = other.string();
            other.m_string = nullptr;
        }
    }

    ~table_entry() {
        if ((m_flags & table_entry_flags::require_destruction) ==
                table_entry_flags::require_destruction) {
            // null pointer checked in operator delete [], so we don't need
            // to check it explicitly

            delete [] m_string;
        }
    }

    std::size_t length() const {
        return m_length;
    }

    const char *string() const {
        if (is_inplace()) {
            return m_inplace_string;
        }

        return m_string;
    }

    bool operator ==(const table_entry &other) const {
        return length() == other.length() && std::memcmp(string(), other.string(), length()) == 0;
    }

 private:
    bool is_inplace() const {
        return (m_flags & table_entry_flags::inplace) == table_entry_flags::inplace;
    }
};
}  // namespace

namespace std {
template<>
struct hash<table_entry> {
    std::size_t operator()(const table_entry &entry) const {
        return Util::Hash::murmur(entry.string(), entry.length());
    }
};
}

namespace {
std::unordered_set<table_entry>& cache() {
    static std::unordered_set<table_entry> g_cache;

    return g_cache;
}

const char *save_to_cache(const char *string, std::size_t length, table_entry_flags flags) {
    if ((flags & table_entry_flags::no_need_copy) == table_entry_flags::no_need_copy) {
        return cache().emplace(string, length, flags).first->string();
    }

    // temporary table_entry, used for searching only. no need to copy string
    auto found = cache().find(table_entry(string, length, table_entry_flags::no_need_copy));

    if (found == cache().end()) {
        return cache().emplace(string, length, flags).first->string();
    }

    return found->string();
}

}  // namespace

void cstring::construct_from_shared(const char *string, std::size_t length) {
    str = save_to_cache(string, length, table_entry_flags::none);
}

void cstring::construct_from_unique(const char *string, std::size_t length) {
    str = save_to_cache(string, length,
        table_entry_flags::no_need_copy | table_entry_flags::require_destruction);
}

void cstring::construct_from_literal(const char *string, std::size_t length) {
    str = save_to_cache(string, length, table_entry_flags::no_need_copy);
}

size_t cstring::cache_size(size_t &count) {
    size_t rv = 0;
    count = cache().size();
    for (auto &s : cache())
        rv += sizeof(s) + s.length();
    return rv;
}

cstring cstring::newline = cstring("\n");
cstring cstring::empty = cstring("");

bool cstring::startsWith(const cstring& prefix) const {
    return size() >= prefix.size() && memcmp(str, prefix.str, prefix.size()) == 0;
}

bool cstring::endsWith(const cstring& suffix) const {
    return size() >= suffix.size() &&
           memcmp(str + size() - suffix.size(), suffix.str, suffix.size()) == 0;
}

cstring cstring::before(const char* at) const {
    return substr(0, at - str);
}

cstring cstring::substr(size_t start, size_t length) const {
    if (size() <= start) return cstring::empty;
    std::string s = str;
    return s.substr(start, length);
}

cstring cstring::replace(char c, char with) const {
    char* dup = strdup(c_str());
    for (char* p = dup; *p; ++p)
        if (*p == c)
            *p = with;
    return cstring(dup);
}

cstring cstring::replace(cstring search, cstring replace) const {
    if (search.isNullOrEmpty() || isNullOrEmpty())
        return *this;

    std::string s_str = str;
    std::string s_search = search.str;
    std::string s_replace = replace.str;

    size_t pos = 0;
    while ((pos = s_str.find(s_search, pos)) != std::string::npos) {
         s_str.replace(pos, s_search.length(), s_replace);
         pos += s_replace.length();
    }
    return cstring(s_str);
}

// See https://stackoverflow.com/a/33799784/4538702
cstring cstring::escapeJson() const {
    std::ostringstream o;
    for (size_t i = 0; i < size(); i++) {
        char c = get(i);
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default: {
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u"
                      << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    o << c;
                }
            }
        }
    }
    return cstring(o.str());
}

cstring cstring::toUpper() {
    std::string st = str;
    std::transform(st.begin(), st.end(), st.begin(), ::toupper);
    cstring ret = cstring::to_cstring(st);
    return ret;
}

cstring cstring::capitalize() {
    std::string st = str;
    st[0] = ::toupper(st[0]);
    cstring ret = cstring::to_cstring(st);
    return ret;
}
