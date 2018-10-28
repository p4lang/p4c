#include "hash.h"
#include <cstdint>
#include <cstring>

namespace Util {
namespace Hash {

namespace Detail {

template<std::size_t Size = sizeof(std::size_t)>
struct fnv1a_traits;

template<>
struct fnv1a_traits<sizeof(std::uint32_t)> {
    static const std::size_t offset_basis = static_cast<std::size_t>(2166136261);
    static const std::size_t prime = static_cast<std::size_t>(16777619);
};

template<>
struct fnv1a_traits<sizeof(std::uint64_t)> {
    static const std::size_t offset_basis = static_cast<std::size_t>(14695981039346656037ULL);
    static const std::size_t prime = static_cast<std::size_t>(1099511628211ULL);
};

}  // namespace Detail

std::size_t fnv1a(const void *data, std::size_t size) {
    auto raw = reinterpret_cast<const unsigned char *>(data);
    std::size_t result = Detail::fnv1a_traits<>::offset_basis;

    for (std::size_t byte = 0; byte < size; ++byte) {
        result ^= (std::size_t)raw[byte];
        result *= Detail::fnv1a_traits<>::prime;
    }

    return result;
}


namespace Detail {
std::uint32_t murmur32(const void* data, std::uint32_t size) {
    const std::uint32_t m = 0x5bd1e995;
    const std::uint64_t seed = 0xc70f6907UL;
    std::uint32_t result = seed ^ size;
    const char* raw = static_cast<const char*>(data);

    while (size >= sizeof(std::uint32_t)) {
        std::uint32_t k;
        std::memcpy(&k, raw, sizeof(k));
        k *= m;
        k ^= k >> 24;
        k *= m;
        result *= m;
        result ^= k;
        raw += 4;
        size -= 4;
    }

    switch (size) {
    case 3:
        result ^= static_cast<unsigned char>(raw[2]) << 16;
        // fallthrough
    case 2:
        result ^= static_cast<unsigned char>(raw[1]) << 8;
        // fallthrough
    case 1:
        result ^= static_cast<unsigned char>(raw[0]);
        result *= m;
        break;
    }

    result ^= result >> 13;
    result *= m;
    result ^= result >> 15;
    return result;
}

std::uint64_t murmur64(const void *data, std::uint64_t size) {
    const std::uint64_t mul = (UINT64_C(0xc6a4a793) << 32) + UINT64_C(0x5bd1e995);

    const std::uint64_t seed = UINT64_C(0xc70f6907);

    const char* const buf = static_cast<const char*>(data);

    const int alignedSize = size & ~0x7;
    const char* const end = buf + alignedSize;

    std::uint64_t hash = seed ^ (size * mul);

    for (const char* p = buf; p != end; p += 8) {
        std::uint64_t k;
        std::memcpy(&k, p, sizeof(k));
        k *= mul;
        k ^= k >> 47;
        const uint64_t data = (k) * mul;
        hash ^= data;
        hash *= mul;
    }

    if (size & 0x7) {
        std::uint64_t data = 0;

        for (std::size_t n = (size & 0x7) - 1;; --n) {
            data = (data << 8) + static_cast<unsigned char>(end[n]);

            if (n == 0) {
                break;
            }
        }

        hash ^= data;
        hash *= mul;
    }

    hash ^= hash >> 47;
    hash *= mul;
    hash ^= hash >> 47;
    return hash;
}

template<std::size_t Size = sizeof(std::size_t)>
struct murmur;

template<>
struct murmur<sizeof(std::uint32_t)> {
    static std::size_t hash(const void *data, std::size_t size) {
        return murmur32(data, size);
    }
};

template<>
struct murmur<sizeof(std::uint64_t)> {
    static std::size_t hash(const void *data, std::size_t size) {
        return murmur64(data, size);
    }
};
}  // namespace Detail

std::size_t murmur(const void *data, std::size_t size) {
    return Detail::murmur<>::hash(data, size);
}
}  // namespace Hash
}  // namespace Util
