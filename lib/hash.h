#ifndef LIB_HASH_H_
#define LIB_HASH_H_

#include <cstddef>
#include <type_traits>

namespace Util::Hash {

namespace helper_ {
// If called with 0, this overload will be preferred -- so if both are enabled, this one will be
// used.  It is preferred becuase there will be no cast.
template <typename T>
constexpr auto has_data_and_size(int)
    -> decltype(std::declval<T>().data(), std::declval<T>().size(), bool()) {
    return true;
}

// This one is not preferred, it is only called if the other is not available
template <typename T>
constexpr auto has_data_and_size(short) -> bool {
    return false;
}
}  // namespace helper_

/// A trait for types that can be trivially hashed by their memory representation. Must be standard
/// layout, trivially copyable and not a pointer.
template <typename T>
constexpr bool trivially_hashable =
    std::is_standard_layout_v<T> && !std::is_pointer_v<T> && std::is_trivially_copyable_v<T>;

/// A trait for hashable types. Either trivially_hashable, or has data() and size() public members.
template <typename T>
constexpr bool hashable = trivially_hashable<T> || helper_::has_data_and_size<T>(0);

std::size_t fnv1a(const void *data, std::size_t size);

/// If the object has public size() and data() forwards these to \ref vnv1a(const void *, size_t).
/// Otherwise if the object is trivally hashable hashes the memory representation of the object.
template <typename T>
auto fnv1a(const T &obj) -> std::enable_if_t<hashable<T>, std::size_t> {
    if constexpr (trivially_hashable<T>) {
        return fnv1a(reinterpret_cast<const void *>(&obj), sizeof(T));
    }
    if constexpr (helper_::has_data_and_size<T>(0)) {
        return fnv1a(obj.data(), obj.size());
    }
}

std::size_t murmur(const void *data, std::size_t size);

/// If the object has public size() and data() forwards these to \ref murmur(const void *, size_t).
/// Otherwise if the object is trivally hashable hashes the memory representation of the object.
template <typename T>
auto murmur(const T &obj) -> std::enable_if_t<hashable<T>, std::size_t> {
    if constexpr (trivially_hashable<T>) {
        return murmur(reinterpret_cast<const void *>(&obj), sizeof(T));
    }
    if constexpr (helper_::has_data_and_size<T>(0)) {
        return murmur(obj.data(), obj.size());
    }
}

}  // namespace Util::Hash

#endif /* LIB_HASH_H_ */
