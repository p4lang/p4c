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
