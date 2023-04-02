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

/* -*-c++-*- */

#ifndef LIB_STRINGIFY_H_
#define LIB_STRINGIFY_H_

#include <stdint.h>

#include <cstdarg>

#include "big_int_util.h"
#include "cstring.h"
#include "stringref.h"

// convert values to cstrings
namespace Util {
template <typename T, typename Enabled = void>
struct hastoString_s {
    static constexpr bool value = false;
};

template <typename T>
struct hastoString_s<T,
                     std::enable_if_t<std::is_member_function_pointer_v<decltype(&T::toString)>>> {
    static constexpr bool value = std::is_member_function_pointer_v<decltype(&T::toString)>;
};

template <typename T>
constexpr bool hasToString() {
    return hastoString_s<T>::value;
}

template <typename T, typename = decltype(std::to_string((T)0))>
cstring toString(T value) {
    return std::to_string(value);
}

template <typename T>
auto toString(const T &value) -> typename std::enable_if<hasToString<T>(), cstring>::type {
    return value.toString();
}

template <typename T>
auto toString(T &value) -> typename std::enable_if<hasToString<T>(), cstring>::type {
    return value.toString();
}

template <typename T>
auto toString(const T *value) -> typename std::enable_if<hasToString<T>(), cstring>::type {
    return value->toString();
}

template <typename T>
auto toString(T *value) -> typename std::enable_if<hasToString<T>(), cstring>::type {
    return value->toString();
}

cstring toString(bool value);
cstring toString(std::string value);
cstring toString(const char *value);
cstring toString(cstring value);
cstring toString(StringRef value);
/// A width of zero indicates that no width should be displayed.
cstring toString(const big_int value, unsigned width, bool sign, unsigned int base = 10);
cstring toString(const void *value);

// printf into a string
cstring printf_format(const char *fmt_str, ...);
// vprintf into a string
cstring vprintf_format(const char *fmt_str, va_list ap);
}  // namespace Util
#endif /* LIB_STRINGIFY_H_ */
