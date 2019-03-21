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

#include <stdarg.h>
#include <sstream>
#include "stringify.h"

namespace Util {
cstring toString(bool value) {
    return value ? cstring::literal("true") : cstring::literal("false");
}

cstring toString(std::string value) {
    return value;
}

cstring toString(const char* value) {
    if (value == nullptr)
        return cstring::literal("<nullptr>");
    return cstring(value);
}

cstring toString(const void* value) {
    if (value == nullptr)
        return cstring::literal("<nullptr>");
    std::stringstream result;
    result << value;
    return result.str();
}

cstring toString(const mpz_class* value, unsigned int base) {
    if (value == nullptr)
        return cstring::literal("<nullptr>");
    mpz_class v = *value;
    std::ostringstream oss;
    if (v < 0) {
        oss << "-";
        v = -v;
    }
    switch (base) {
        case 2:
            oss << "0b";
            break;
        case 8:
            oss << "0o";
            break;
        case 16:
            oss << "0x";
            break;
        case 10:
            break;
        default:
            throw std::runtime_error("Unexpected base");
    }
    oss << v.get_str(static_cast<int>(base));
    return oss.str();
}

cstring toString(cstring value) {
    if (value.isNull())
        return cstring::literal("<nullptr>");
    return value;
}

cstring toString(StringRef value) {
    return value;
}

cstring printf_format(const char* fmt_str, ...) {
    if (fmt_str == nullptr)
        throw std::runtime_error("Null format string");
    va_list ap;
    va_start(ap, fmt_str);
    cstring formatted = vprintf_format(fmt_str, ap);
    va_end(ap);
    return formatted;
}

// printf into a string
cstring vprintf_format(const char* fmt_str, va_list ap) {
    static char buf[128];
    va_list ap_copy;
    va_copy(ap_copy, ap);
    if (fmt_str == nullptr)
        throw std::runtime_error("Null format string");

    int size = vsnprintf(buf, sizeof(buf), fmt_str, ap);
    if (size < 0)
        throw std::runtime_error("Error in vsnprintf");
    if (static_cast<size_t>(size) >= sizeof(buf)) {
        char* formatted = new char[size + 1];
        vsnprintf(formatted, size + 1, fmt_str, ap_copy);
        return cstring::own(formatted, size);
    }
    va_end(ap_copy);
    return cstring(buf);
}

}  // namespace Util
