#ifndef LIB_HASH_H_
#define LIB_HASH_H_

#include <cstddef>
#include <type_traits>

namespace Util {
uint64_t hash(const void *data, std::size_t size);

template <typename T>
auto hash(
    const T &obj,
    typename std::enable_if_t<std::is_standard_layout_v<T> && !std::is_pointer_v<T>> * = nullptr) {
    return hash(reinterpret_cast<const void *>(&obj), sizeof(T));
}
}  // namespace Util

#endif /* LIB_HASH_H_ */
