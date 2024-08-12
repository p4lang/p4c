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

#include "absl/container/node_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"

#if HAVE_LIBGC
#include <gc_cpp.h>
#define IF_HAVE_LIBGC(X) X
#else
#define IF_HAVE_LIBGC(X)
#endif /* HAVE_LIBGC */

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

#include "hash.h"

namespace {
enum class table_entry_flags {
    none,
    no_need_copy = 1 << 0,
    require_destruction = 1 << 1,
    inplace = 1 << 2,
};

inline table_entry_flags operator&(table_entry_flags l, table_entry_flags r) {
    return static_cast<table_entry_flags>(static_cast<int>(l) & static_cast<int>(r));
}

inline table_entry_flags operator|(table_entry_flags l, table_entry_flags r) {
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
            // Make copy of string elsewhere
            auto copy = new IF_HAVE_LIBGC((NoGC)) char[length + 1];
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

            delete[] m_string;
        }
    }

    std::size_t length() const { return m_length; }

    const char *string() const {
        if (is_inplace()) {
            return m_inplace_string;
        }

        return m_string;
    }

    bool operator==(const table_entry &other) const {
        return length() == other.length() && std::memcmp(string(), other.string(), length()) == 0;
    }

    bool operator==(std::string_view other) const {
        return length() == other.length() && std::memcmp(string(), other.data(), length()) == 0;
    }

 private:
    bool is_inplace() const {
        return (m_flags & table_entry_flags::inplace) == table_entry_flags::inplace;
    }
};

// We'd make Util::Hash to be transparent instead. However, this would enable
// transparent hashing globally and in some cases in very undesired manner. So
// for now aim for more fine-grained approach.
struct TableEntryHash {
    using is_transparent = void;

    // IMPORTANT: These two hashes MUST match in order for heterogenous
    // lookup to work properly
    size_t operator()(const table_entry &entry) const {
        return Util::hash(entry.string(), entry.length());
    }

    size_t operator()(std::string_view entry) const {
        return Util::hash(entry.data(), entry.length());
    }
};

auto &cache() {
    // We need node_hash_set due to SSO: we return address of embedded string
    // that should be stable
    static absl::node_hash_set<table_entry, TableEntryHash, std::equal_to<>> g_cache;

    return g_cache;
}

const char *save_to_cache(const char *string, std::size_t length, table_entry_flags flags) {
    // Checks if string is already cached and if not, calls ctor to construct in
    // place.  As a result, only a single lookup is performed regardless whether
    // entry is in cache or not.
    return cache()
        .lazy_emplace(table_entry(string, length, table_entry_flags::no_need_copy),
                      [string, length, flags](const auto &ctor) { ctor(string, length, flags); })
        ->string();
}

}  // namespace

bool cstring::is_cached(std::string_view s) { return cache().contains(s); }

cstring cstring::get_cached(std::string_view s) {
    auto entry = cache().find(s);
    if (entry == cache().end()) return nullptr;

    cstring res;
    res.str = entry->string();

    return res;
}

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
    for (auto &s : cache()) rv += sizeof(s) + s.length();
    return rv;
}

bool cstring::startsWith(std::string_view prefix) const {
    if (prefix.empty()) return true;
    return size() >= prefix.size() && memcmp(str, prefix.data(), prefix.size()) == 0;
}

bool cstring::endsWith(std::string_view suffix) const {
    if (suffix.empty()) return true;
    return size() >= suffix.size() &&
           memcmp(str + size() - suffix.size(), suffix.data(), suffix.size()) == 0;
}

cstring cstring::before(const char *at) const { return substr(0, at - str); }

cstring cstring::substr(size_t start, size_t length) const {
    if (size() <= start) return cstring::empty;
    return cstring(string_view().substr(start, length));
}

cstring cstring::replace(char c, char with) const {
    if (isNullOrEmpty()) return *this;
    char *dup = strdup(c_str());
    for (char *p = dup; *p; ++p)
        if (*p == c) *p = with;
    return cstring(dup);
}

cstring cstring::replace(std::string_view search, std::string_view replace) const {
    if (search.empty() || isNullOrEmpty()) return *this;

    return cstring(absl::StrReplaceAll(str, {{search, replace}}));
}

cstring cstring::trim(const char *ws) const {
    if (isNullOrEmpty()) return *this;

    const char *start = str + strspn(str, ws);
    size_t len = strlen(start);
    while (len > 0 && strchr(ws, start[len - 1])) --len;
    return cstring(start, len);
}

cstring cstring::indent(size_t amount) const {
    std::string spaces(amount, ' ');
    std::string spc = "\n" + spaces;
    return cstring(absl::StrCat(spaces, absl::StrReplaceAll(string_view(), {{"\n", spc}})));
}

// See https://stackoverflow.com/a/33799784/4538702
cstring cstring::escapeJson() const {
    std::ostringstream o;
    for (size_t i = 0; i < size(); i++) {
        char c = get(i);
        switch (c) {
            case '"':
                o << "\\\"";
                break;
            case '\\':
                o << "\\\\";
                break;
            case '\b':
                o << "\\b";
                break;
            case '\f':
                o << "\\f";
                break;
            case '\n':
                o << "\\n";
                break;
            case '\r':
                o << "\\r";
                break;
            case '\t':
                o << "\\t";
                break;
            default: {
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                      << static_cast<int>(c);
                } else {
                    o << c;
                }
            }
        }
    }
    return cstring(o.str());
}

cstring cstring::toUpper() const {
    std::string st = str;
    std::transform(st.begin(), st.end(), st.begin(), ::toupper);
    return cstring(st);
}

cstring cstring::toLower() const {
    std::string st = str;
    std::transform(st.begin(), st.end(), st.begin(), ::tolower);
    return cstring(st);
}

cstring cstring::capitalize() const {
    std::string st = str;
    st[0] = ::toupper(st[0]);
    return cstring(st);
}
