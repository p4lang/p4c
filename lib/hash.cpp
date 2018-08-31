#include "hash.h"
#include <cstdint>

namespace Util {
namespace Hash {

namespace Detail {

template<std::size_t Size = sizeof(std::size_t)>
struct fnv1a_traits;

template<>
struct fnv1a_traits<sizeof(std::uint32_t)>
{
    static const std::size_t offset_basis = static_cast<std::size_t>(2166136261);
    static const std::size_t prime = static_cast<std::size_t>(16777619);
};

template<>
struct fnv1a_traits<sizeof(std::uint64_t)>
{
    static const std::size_t offset_basis = static_cast<std::size_t>(14695981039346656037ULL);
    static const std::size_t prime = static_cast<std::size_t>(1099511628211ULL);
};

} // namespace Detail

std::size_t fnv1a(const void *data, std::size_t size) {
    auto raw = reinterpret_cast<const unsigned char *>(data);
    std::size_t result = Detail::fnv1a_traits<>::offset_basis;

    for (std::size_t byte = 0; byte < size; ++byte) {
        result ^= (std::size_t)raw[byte];
        result *= Detail::fnv1a_traits<>::prime;
    }

    return result;
}

} // namespace Hash
} // namespace Util
