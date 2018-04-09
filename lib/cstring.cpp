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
#include <string>
#include <unordered_set>

static std::unordered_set<std::string> *cache = nullptr;

cstring &cstring::operator=(const char *p) {
    if (cache == nullptr)
        cache = new std::unordered_set<std::string>();
    str = p ? cache->emplace(p).first->c_str() : 0;
    return *this;
}

cstring& cstring::operator=(const std::string& s) {
    if (cache == nullptr)
        cache = new std::unordered_set<std::string>();
    str = cache->insert(s).first->c_str();
    return *this;
}

size_t cstring::cache_size(size_t &count) {
    size_t rv = 0;
    if (cache) {
        count = cache->size();
        for (auto &s : *cache)
            rv += sizeof(s) + s.size();
    } else {
        count = 0;
    }
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

cstring::cstring(const std::stringstream & str) : cstring(str.str()) {}

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

cstring cstring::escapeJson() const {
    std::string out;
    for (size_t i = 0; i < size(); i++) {
        char c = get(i);
        if (c == '\\' || c == '"')
            out += "\\";
        out += c;
    }
    return cstring(out);
}
