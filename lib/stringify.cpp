#include <stdarg.h>
#include "stringify.h"

namespace Util {
cstring toString(bool value) {
    return value ? "true" : "false";
}

cstring toString(int value) {
    return std::to_string(value);
}

cstring toString(long value) {
    return std::to_string(value);
}

cstring toString(uint64_t value) {
    return std::to_string(value);
}

cstring toString(unsigned value) {
    return std::to_string(value);
}

cstring toString(double value) {
    return std::to_string(value);
}

cstring toString(std::string value) {
    return value;
}

cstring toString(const char* value) {
    if (value == nullptr)
        return cstring("<nullptr>");
    return cstring(value);
}

cstring toString(const void* value) {
    if (value == nullptr)
        return cstring("<nullptr>");
    std::stringstream result;
    result << value;
    return result.str();
}

cstring toString(const mpz_class* value) {
    if (value == nullptr)
        return cstring("<nullptr>");
    return cstring(value->get_str());
}

cstring toString(cstring value) {
    if (value.isNull())
        return cstring("<nullptr>");
    return value;
}

cstring toString(StringRef value) {
    return value;
}

cstring printf_format(const char* fmt_str, ...) {
    if (fmt_str == nullptr)
        throw std::runtime_error("Null format string");

    int final_n, n = (strlen(fmt_str)) * 2;
    /* Reserve two times as much as the length of the fmt_str */
    char* formatted;
    va_list ap;
    while (1) {
        formatted = new char[n];
        strncpy(formatted, fmt_str, n);
        va_start(ap, fmt_str);
        final_n = vsnprintf(formatted, n, fmt_str, ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return cstring(formatted);
}

// printf into a string
cstring vprintf_format(const char* fmt_str, va_list ap) {
    static char buf[128];
    if (fmt_str == nullptr)
        throw std::runtime_error("Null format string");

    int size = vsnprintf(buf, sizeof(buf), fmt_str, ap);
    if (size < 0)
        throw std::runtime_error("Error in vsnprintf");
    if (static_cast<size_t>(size) >= sizeof(buf)) {
        char* formatted = new char[size + 1];
        vsnprintf(formatted, size, fmt_str, ap);
        return cstring(formatted);
    }
    return cstring(buf);
}

}  // namespace Util
