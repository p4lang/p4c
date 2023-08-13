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

#include "stringify.h"

#include <stdio.h>

#include <algorithm>
#include <deque>
#include <sstream>
#include <stdexcept>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "exceptions.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/stringref.h"

namespace Util {
cstring toString(bool value) {
    return value ? cstring::literal("true") : cstring::literal("false");
}

cstring toString(std::string value) { return value; }

cstring toString(const char *value) {
    if (value == nullptr) return cstring::literal("<nullptr>");
    return cstring(value);
}

cstring toString(const void *value) {
    if (value == nullptr) return cstring::literal("<nullptr>");
    std::stringstream result;
    result << value;
    return result.str();
}

char DigitToChar(int digit) {
    switch (digit) {
        case 0:
            return '0';
        case 1:
            return '1';
        case 2:
            return '2';
        case 3:
            return '3';
        case 4:
            return '4';
        case 5:
            return '5';
        case 6:
            return '6';
        case 7:
            return '7';
        case 8:
            return '8';
        case 9:
            return '9';
        case 10:
            return 'a';
        case 11:
            return 'b';
        case 12:
            return 'c';
        case 13:
            return 'd';
        case 14:
            return 'e';
        case 15:
            return 'f';
        default:
            break;
    }
    BUG("Unexpected digit: %1%", digit);
}

cstring toString(big_int value, unsigned width, bool sign, unsigned int base) {
    std::ostringstream oss;
    big_int zero = 0;
    if (value < zero) {
        oss << "-";
        value = -value;
    }

    if (width) {
        oss << width;
        oss << (sign ? "s" : "w");
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
            BUG("Unexpected base %1%", base);
    }
    std::deque<char> buf;
    do {
        const int digit = static_cast<int>(static_cast<big_int>(value % base));
        value /= base;
        buf.push_front(DigitToChar(digit));
    } while (value > 0);
    for (auto ch : buf) oss << ch;
    return oss.str();
}

cstring toString(cstring value) {
    if (value.isNull()) return cstring::literal("<nullptr>");
    return value;
}

cstring toString(StringRef value) { return value; }

cstring printf_format(const char *fmt_str, ...) {
    if (fmt_str == nullptr) throw std::runtime_error("Null format string");
    va_list ap;
    va_start(ap, fmt_str);
    cstring formatted = vprintf_format(fmt_str, ap);
    va_end(ap);
    return formatted;
}

// printf into a string
cstring vprintf_format(const char *fmt_str, va_list ap) {
    static char buf[128];
    va_list ap_copy;
    va_copy(ap_copy, ap);
    if (fmt_str == nullptr) throw std::runtime_error("Null format string");

    int size = vsnprintf(buf, sizeof(buf), fmt_str, ap);
    if (size < 0) throw std::runtime_error("Error in vsnprintf");
    if (static_cast<size_t>(size) >= sizeof(buf)) {
        char *formatted = new char[size + 1];
        vsnprintf(formatted, size + 1, fmt_str, ap_copy);
        return cstring::own(formatted, size);
    }
    va_end(ap_copy);
    return cstring(buf);
}

}  // namespace Util
