#include <assert.h>
#include <stdint.h>
#include "hex.h"

std::ostream &operator<<(std::ostream &os, const hexvec &h) {
    auto save = os.flags();
    char *p = reinterpret_cast<char *>(h.data);
    for (size_t i = 0; i < h.len; i++, p += h.elsize) {
        os << (i ? ' ' : '[');
        uintmax_t val;
        switch (h.elsize) {
        case 1:
            val = *reinterpret_cast<uint8_t *>(p);
            break;
        case 2:
            val = *reinterpret_cast<uint16_t *>(p);
            break;
        case 4:
            val = *reinterpret_cast<uint32_t *>(p);
            break;
        case 8:
            val = *reinterpret_cast<uint64_t *>(p);
            break;
        default:
            val = *reinterpret_cast<uintmax_t *>(p);
            val &= ~(~0ULL << 8*h.elsize); }
        os << std::hex << std::setw(h.width) << std::setfill(h.fill) << val; }
    os << ']';
    os.flags(save);
    return os;
}
