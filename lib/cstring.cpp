#include "cstring.h"
#include <gc/gc.h>
#include <string>
#include <unordered_set>

cstring &cstring::operator=(const char *p) {
#if HAVE_LIBGC
    /* DANGER -- on OSX, can't safely call the garbage collector allocation
     * routines from a static global constructor without manually initializing
     * it first.  Since we have a couple of global static cstrings, we need
     * to force initialization */
    static bool init = false;
    if (!init) {
        GC_INIT();
        init = true; }
#endif
    static std::unordered_set<std::string> *cache = nullptr;
    if (cache == nullptr)
        cache = new std::unordered_set<std::string>();
    str = p ? cache->emplace(p).first->c_str() : 0;
    return *this;
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
