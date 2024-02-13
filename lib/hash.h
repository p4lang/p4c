#ifndef LIB_HASH_H_
#define LIB_HASH_H_

#include <cstddef>
#include <type_traits>

namespace Util {

namespace Detail {
constexpr uint64_t PRIME64_1 = UINT64_C(11400714785074694791);
constexpr uint64_t PRIME64_2 = UINT64_C(14029467366897019727);
constexpr uint64_t PRIME64_3 = UINT64_C(1609587929392839161);
constexpr uint64_t PRIME64_4 = UINT64_C(9650029242287828579);
constexpr uint64_t PRIME64_5 = UINT64_C(2870177450012600261);

constexpr uint64_t PRIME_MX1 = UINT64_C(0x165667919E3779F9);
constexpr uint64_t PRIME_MX2 = UINT64_C(0x9FB21C651E98DF25);

static uint64_t XXH64_avalanche(uint64_t hash) {
    hash ^= hash >> 33;
    hash *= PRIME64_2;
    hash ^= hash >> 29;
    hash *= PRIME64_3;
    hash ^= hash >> 32;
    return hash;
}

static uint64_t rotl64(uint64_t X, size_t R) { return (X << R) | (X >> (64 - R)); }

static uint64_t XXH3_rrmxmx(uint64_t acc, uint64_t len) {
    acc ^= rotl64(acc, 49) ^ rotl64(acc, 24);
    acc *= PRIME_MX2;
    acc ^= (acc >> 35) + len;
    acc *= PRIME_MX2;
    return acc ^ (acc >> 28);
}

}  // namespace Detail

uint64_t hash(const void *data, std::size_t size);

static inline uint64_t hash_avalanche(uint64_t hash) { return Detail::XXH64_avalanche(hash); }

static inline uint64_t hash_combine(uint64_t lhs, uint64_t rhs) {
    return Detail::XXH3_rrmxmx(lhs, rhs);
}

template <typename T>
auto hash(
    const T &obj,
    typename std::enable_if_t<std::is_standard_layout_v<T> && !std::is_pointer_v<T>> * = nullptr) {
    return hash(reinterpret_cast<const void *>(&obj), sizeof(T));
}
}  // namespace Util

#endif /* LIB_HASH_H_ */
