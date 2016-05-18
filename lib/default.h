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

#ifndef P4C_LIB_DEFAULT_H_
#define P4C_LIB_DEFAULT_H_

#include <type_traits>

namespace Util {

// Generates default values of different types
template<typename T>
auto Default() ->
        typename std::enable_if<std::is_pointer<T>::value, T>::type
{ return nullptr; }

template<typename T>
auto Default() ->
        typename std::enable_if<std::is_class<T>::value, T>::type
{ return T(); }

template<typename T>
auto Default() ->
        typename std::enable_if<std::is_arithmetic<T>::value, T>::type
{ return static_cast<T>(0); }

}  // namespace Util

#endif /* P4C_LIB_DEFAULT_H_ */
