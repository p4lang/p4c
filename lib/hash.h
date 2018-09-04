#ifndef _LIB_HASH_H_
#define _LIB_HASH_H_

#include <cstddef>
#include <type_traits>

namespace Util {
namespace Hash {
std::size_t fnv1a(const void *data, std::size_t size);

// returns fnv1a hash sum for object with public methods size() and data()
template<typename T>
auto fnv1a(const T& obj) -> decltype(fnv1a(obj.data(), obj.size())) {
    return fnv1a(obj.data(), obj.size());
}

// returns fnv1a hash sum for object with standard layout
template<typename T>
auto fnv1a(const T& obj, typename std::enable_if<
        std::is_standard_layout<T>::value && !std::is_pointer<T>::value>::type * = nullptr)
    -> decltype(fnv1a(reinterpret_cast<const void *>(&obj), sizeof(T))) {
    return fnv1a(reinterpret_cast<const void *>(&obj), sizeof(T));
}

std::size_t murmur(const void *data, std::size_t size);

// returns Murmur hash sum for object with public methods size() and data()
template<typename T>
auto murmur(const T& obj) -> decltype(murmur(obj.data(), obj.size())) {
    return murmur(obj.data(), obj.size());
}

// returns fnv1a hash sum for object with standard layout
template<typename T>
auto murmur(const T& obj, typename std::enable_if<
        std::is_standard_layout<T>::value && !std::is_pointer<T>::value>::type * = nullptr)
    -> decltype(murmur(reinterpret_cast<const void *>(&obj), sizeof(T))) {
    return murmur(reinterpret_cast<const void *>(&obj), sizeof(T));
}
}  // namespace Hash
}  // namespace Util

#endif /* _LIB_HASH_H_ */
