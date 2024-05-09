#ifndef LIB_HASH_H_
#define LIB_HASH_H_

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace Util {

namespace Detail {
constexpr uint32_t PRIME32_1 = UINT32_C(0x9E3779B1);
constexpr uint32_t PRIME32_2 = UINT32_C(0x85EBCA77);
constexpr uint32_t PRIME32_3 = UINT32_C(0xC2B2AE3D);

constexpr uint64_t PRIME64_1 = UINT64_C(11400714785074694791);
constexpr uint64_t PRIME64_2 = UINT64_C(14029467366897019727);
constexpr uint64_t PRIME64_3 = UINT64_C(1609587929392839161);
constexpr uint64_t PRIME64_4 = UINT64_C(9650029242287828579);
constexpr uint64_t PRIME64_5 = UINT64_C(2870177450012600261);

constexpr uint64_t PRIME_MX1 = UINT64_C(0x165667919E3779F9);
constexpr uint64_t PRIME_MX2 = UINT64_C(0x9FB21C651E98DF25);

static constexpr uint32_t XXH32_avalanche(uint32_t hash) {
    hash ^= hash >> 15;
    hash *= PRIME32_2;
    hash ^= hash >> 13;
    hash *= PRIME32_3;
    hash ^= hash >> 16;
    return hash;
}

static constexpr uint64_t XXH64_avalanche(uint64_t hash) {
    hash ^= hash >> 33;
    hash *= PRIME64_2;
    hash ^= hash >> 29;
    hash *= PRIME64_3;
    hash ^= hash >> 32;
    return hash;
}

static constexpr uint64_t rotl64(uint64_t X, size_t R) { return (X << R) | (X >> (64 - R)); }

static constexpr uint64_t XXH3_rrmxmx(uint64_t acc, uint64_t len) {
    acc ^= rotl64(acc, 49) ^ rotl64(acc, 24);
    acc *= PRIME_MX2;
    acc ^= (acc >> 35) + len;
    acc *= PRIME_MX2;
    return acc ^ (acc >> 28);
}

template <typename Int>
struct IntegerHasher {
    constexpr size_t operator()(const Int &i) const noexcept {
        static_assert(sizeof(Int) <= 16, "unsupported input type");
        if constexpr (sizeof(Int) <= 4) {
            return static_cast<size_t>(XXH32_avalanche(static_cast<uint32_t>(i)));
        } else if constexpr (sizeof(Int) <= 8) {
            return static_cast<size_t>(XXH64_avalanche(static_cast<uint64_t>(i)));
        } else {
            using unsigned_type = std::make_unsigned_t<Int>;
            auto const u = static_cast<unsigned_type>(i);
            auto const hi = static_cast<uint64_t>(u >> sizeof(Int) * 4);
            auto const lo = static_cast<uint64_t>(u);
            return static_cast<size_t>(XXH3_rrmxmx(hi, lo));
        }
    }
};

template <typename Float>
struct FloatHasher {
    size_t operator()(const Float &f) const noexcept {
        static_assert(sizeof(Float) <= 8, "unsupported input type");

        // Ensure 0 and -0 get the same hash value
        if (f == Float{}) return 0;

        uint64_t u64 = 0;
        memcpy(&u64, &f, sizeof(Float));
        return static_cast<size_t>(XXH64_avalanche(u64));
    }
};

class StdHasher {
 public:
    template <typename T>
    size_t operator()(const T &t) const noexcept(noexcept(std::hash<T>()(t))) {
        return std::hash<T>()(t);
    }
};
}  // namespace Detail

uint64_t hash(const void *data, std::size_t size);

static inline uint64_t hash_avalanche(uint64_t hash) { return Detail::XXH64_avalanche(hash); }

static inline uint64_t hash_combine(uint64_t lhs, uint64_t rhs) {
    return Detail::XXH3_rrmxmx(lhs, rhs);
}

template <typename T>
auto hash(const T &obj) -> decltype(hash(obj.data(), obj.size())) {
    return hash(obj.data(), obj.size());
}

/// The implementation below decouples generic hash interface provider from a
/// particular implementation for a given class `T`.  One should
/// partially-specialize Util::Hasher<T> to provide a hash value for
/// `T`. Util::Hash is intended to be used instead of std::hash<T> in various
/// associative containers. For example `std::unordered_set<K, Util::Hash>`
/// would work as soon as there is an implementation of Util::Hasher<K>.  The
/// code below provides some default specializations for certain built-in types
/// and theirs combinations (pairs and tuples). Additionally, hashes could be
/// combined either directly (Hash::operator() is variadic) or via dedicated
/// `hash_combine` / `hash_combine_generic` functions.
template <class Key, class Enable = void>
struct Hasher;

struct Hash {
    template <class T>
    constexpr size_t operator()(const T &v) const noexcept(noexcept(Hasher<T>()(v))) {
        return Hasher<T>()(v);
    }

    template <class T, class... Types>
    constexpr size_t operator()(const T &t, const Types &...ts) const {
        return hash_combine((*this)(t), (*this)(ts...));
    }

    constexpr size_t operator()() const noexcept { return 0; }
};

template <>
struct Hasher<bool> {
    constexpr size_t operator()(bool val) const noexcept {
        return val ? std::numeric_limits<size_t>::max() : 0;
    }
};

template <>
struct Hasher<unsigned long long> : Detail::IntegerHasher<unsigned long long> {};

template <>
struct Hasher<signed long long> : Detail::IntegerHasher<signed long long> {};

template <>
struct Hasher<unsigned long> : Detail::IntegerHasher<unsigned long> {};

template <>
struct Hasher<signed long> : Detail::IntegerHasher<signed long> {};

template <>
struct Hasher<unsigned int> : Detail::IntegerHasher<unsigned int> {};

template <>
struct Hasher<signed int> : Detail::IntegerHasher<signed int> {};

template <>
struct Hasher<unsigned short> : Detail::IntegerHasher<unsigned short> {};

template <>
struct Hasher<signed short> : Detail::IntegerHasher<signed short> {};

template <>
struct Hasher<unsigned char> : Detail::IntegerHasher<unsigned char> {};

template <>
struct Hasher<signed char> : Detail::IntegerHasher<signed char> {};

template <>
struct Hasher<char> : Detail::IntegerHasher<char> {};

template <>
struct Hasher<float> : Detail::FloatHasher<float> {};

template <>
struct Hasher<double> : Detail::FloatHasher<double> {};

template <>
struct Hasher<std::string> {
    size_t operator()(const std::string &val) const {
        return static_cast<size_t>(hash(val.data(), val.size()));
    }
};

template <>
struct Hasher<std::string_view> {
    size_t operator()(const std::string_view &val) const {
        return static_cast<size_t>(hash(val.data(), val.size()));
    }
};

template <typename T1, typename T2>
struct Hasher<std::pair<T1, T2>> {
    size_t operator()(const std::pair<T1, T2> &val) const { return Hash()(val.first, val.second); }
};
template <typename... Types>
struct Hasher<std::tuple<Types...>> {
    size_t operator()(const std::tuple<Types...> &val) const { return apply(Hash(), val); }
};

/// In general, pointers are bad hashes: their low 2-3 bits are zero, likewise
/// for the upper bits depending on the ABI. Also, the middle bits might not
/// have enough entropy as addresses come from some common pool. To solve this
/// problem we just use a single iteration of hash_avalanche to improve mixing
/// (see Detail::IntegerHasher).
template <typename T>
struct Hasher<T *> {
    // FIXME: better use std::bit_cast from C++20
    size_t operator()(T *val) const { return Hash()(reinterpret_cast<std::uintptr_t>(val)); }
};

template <typename T>
struct Hasher<std::unique_ptr<T>> {
    size_t operator()(const std::unique_ptr<T> &val) const { return Hash()(val.get()); }
};

template <typename T>
struct Hasher<std::shared_ptr<T>> {
    size_t operator()(const std::shared_ptr<T> &val) const { return Hash()(val.get()); }
};

/// A generic way to create a single hash from multiple hashable
/// objects. hash_combine_generic takes a class Hasher<T> that provides hash
/// value via operator(); hash_combine uses a default hasher StdHasher that uses
/// std::hash<T>. hash_combine_generic hashes each argument and combines those
/// hashes in an order-dependent way to yield a new hash; hash_range does so
/// (also in an order-dependent way) for items in the range [begin, end);

template <class Iter, class Hash = std::hash<typename std::iterator_traits<Iter>::value_type>>
uint64_t hash_range(Iter begin, Iter end, uint64_t hash = 0, Hash Hasher = Hash()) {
    for (; begin != end; ++begin) hash = hash_combine(hash, Hasher(*begin));

    return hash;
}

template <class Hasher>
inline size_t hash_combine_generic(const Hasher &) noexcept {
    return 0;
}

template <class Hasher, typename T, typename... Types>
size_t hash_combine_generic(const Hasher &h, const T &t, const Types &...ts) {
    size_t seed = h(t);
    if (sizeof...(ts) == 0) return seed;

    size_t remainder = hash_combine_generic(h, ts...);
    if constexpr (sizeof(size_t) == sizeof(uint32_t))
        return static_cast<size_t>(
            hash_combine(static_cast<uint64_t>(seed) << 32, static_cast<uint64_t>(remainder)));
    else
        return static_cast<size_t>(hash_combine(seed, remainder));
}

template <typename T, typename... Types>
size_t hash_combine(const T &t, const Types &...ts) {
    return hash_combine_generic(Detail::StdHasher{}, t, ts...);
}

namespace Detail {
template <size_t index, typename... Types>
struct TupleHasher {
    size_t operator()(std::tuple<Types...> const &val) const {
        return hash_combine(TupleHasher<index - 1, Types...>()(val), std::get<index>(val));
    }
};

template <typename... Types>
struct TupleHasher<0, Types...> {
    size_t operator()(std::tuple<Types...> const &val) const {
        return hash_combine(std::get<0>(val));
    }
};
}  // namespace Detail
}  // namespace Util

namespace std {
template <typename T1, typename T2>
struct hash<std::pair<T1, T2>> {
    size_t operator()(const std::pair<T1, T2> &x) const {
        return Util::hash_combine(x.first, x.second);
    }
};

template <typename... Types>
struct hash<std::tuple<Types...>> {
 public:
    size_t operator()(std::tuple<Types...> const &key) const {
        Util::Detail::TupleHasher<sizeof...(Types) - 1, Types...> hasher;

        return hasher(key);
    }
};
}  // namespace std

#endif /* LIB_HASH_H_ */
