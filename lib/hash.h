#ifndef LIB_HASH_H_
#define LIB_HASH_H_

#include <cstddef>
#include <type_traits>

namespace Util {
namespace Hash {
std::size_t murmur(const void *data, std::size_t size);

// returns Murmur hash sum for object with public methods size() and data()
template <typename T>
auto murmur(const T &obj) -> decltype(murmur(obj.data(), obj.size())) {
    return murmur(obj.data(), obj.size());
}

// returns fnv1a hash sum for object with standard layout
template <typename T>
auto murmur(const T &obj, typename std::enable_if<std::is_standard_layout<T>::value &&
                                                  !std::is_pointer<T>::value>::type * = nullptr)
    -> decltype(murmur(reinterpret_cast<const void *>(&obj), sizeof(T))) {
    return murmur(reinterpret_cast<const void *>(&obj), sizeof(T));
}
}  // namespace Hash
}  // namespace Util

#endif /* LIB_HASH_H_ */
