/* -*-c++-*- */

#ifndef P4C_LIB_STRINGIFY_H_
#define P4C_LIB_STRINGIFY_H_

#include <stdint.h>
#include <gmpxx.h>

#include "cstring.h"
#include "stringref.h"

// convert values to cstrings
namespace Util {
// Check whether type T has a method with signature
// cstring toString() const
template<typename T>
class HasToString final {
    template <typename U, cstring (U::*)() const> struct Check;
    template <typename U> static char func(Check<U, &U::toString> *);
    template <typename U> static int func(...);

 public:
    typedef HasToString type;
    enum { value = sizeof(func<T>(0)) == sizeof(char) };
};

cstring toString(bool value);
cstring toString(int value);
cstring toString(long value);
cstring toString(uint64_t value);
cstring toString(unsigned value);
cstring toString(double value);
cstring toString(std::string value);
cstring toString(const char* value);
cstring toString(cstring value);
cstring toString(StringRef value);
cstring toString(const mpz_class* value);
cstring toString(const void* value);

template<typename T>
auto toString(const T& value) -> typename std::enable_if<HasToString<T>::value, cstring>::type
{ return value.toString(); }

// printf into a string
cstring printf_format(const char* fmt_str, ...);
// vprintf into a string
cstring vprintf_format(const char* fmt_str, va_list ap);
}  // namespace Util
#endif /* P4C_LIB_STRINGIFY_H_ */
