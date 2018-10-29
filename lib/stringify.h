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

#ifndef P4C_LIB_STRINGIFY_H_
#define P4C_LIB_STRINGIFY_H_

#include <stdint.h>
#include <gmpxx.h>  // for mpz_class
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

template<typename T, typename = decltype(std::to_string((T)0))>
cstring toString(T value) { return std::to_string(value); }

template<typename T>
auto toString(const T& value) -> typename std::enable_if<HasToString<T>::value, cstring>::type
{ return value.toString(); }

template<typename T>
auto toString(T& value) -> typename std::enable_if<HasToString<T>::value, cstring>::type
{ return value.toString(); }

template<typename T>
auto toString(const T* value) -> typename std::enable_if<HasToString<T>::value, cstring>::type
{ return value->toString(); }

template<typename T>
auto toString(T* value) -> typename std::enable_if<HasToString<T>::value, cstring>::type
{ return value->toString(); }

cstring toString(bool value);
cstring toString(std::string value);
cstring toString(const char* value);
cstring toString(cstring value);
cstring toString(StringRef value);
cstring toString(const mpz_class* value);
cstring toString(const void* value);

// printf into a string
cstring printf_format(const char* fmt_str, ...);
// vprintf into a string
cstring vprintf_format(const char* fmt_str, va_list ap);
}  // namespace Util
#endif /* P4C_LIB_STRINGIFY_H_ */
