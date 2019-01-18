/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "bitvec.h"
#include "hex.h"

std::ostream &operator<<(std::ostream &os, const bitvec &bv) {
    if (bv.size == 1) {
        os << hex(bv.data);
    } else {
        bool first = true;
        for (int i = bv.size-1; i >= 0; i--) {
            if (first) {
                if (!bv.ptr[i]) continue;
                os << hex(bv.ptr[i]);
                first = false;
            } else {
                os << hex(bv.ptr[i], sizeof(bv.data)*2, '0'); } }
        if (first)
            os << '0';
    }
    return os;
}

bitvec &bitvec::operator>>=(size_t count) {
    if (size == 1) {
        if (count >= bits_per_unit)
            data = 0;
        else
            data >>= count;
        return *this; }
    int off = count / bits_per_unit;
    count %= bits_per_unit;
    for (size_t i = 0; i < size; i++)
        if (i + off < size) {
            ptr[i] = ptr[i+off] >> count;
            if (count && i + off + 1 < size)
                ptr[i] |= ptr[i+off+1] << (bits_per_unit - count);
        } else {
            ptr[i] = 0; }
    while (size > 1 && !ptr[size-1]) size--;
    if (size == 1) {
        auto tmp = ptr[0];
        delete [] ptr;
        data = tmp; }
    return *this;
}

bitvec &bitvec::operator<<=(size_t count) {
    size_t needsize = (max().index() + count + bits_per_unit)/bits_per_unit;
    if (needsize > size) expand(needsize);
    if (size == 1) {
        data <<= count;
        return *this; }
    int off = count / bits_per_unit;
    count %= bits_per_unit;
    for (int i = size-1; i >= 0; i--)
        if (i >= off) {
            ptr[i] = ptr[i-off] << count;
            if (count && i > off)
                ptr[i] |= ptr[i-off-1] >> (bits_per_unit - count);
        } else {
            ptr[i] = 0; }
    return *this;
}

bitvec bitvec::getslice(size_t idx, size_t sz) const {
    if (sz == 0) return bitvec();
    if (idx >= size * bits_per_unit) return bitvec();
    if (idx + sz > size * bits_per_unit)
        sz = size * bits_per_unit - idx;
    if (size > 1) {
        bitvec rv;
        unsigned shift = idx % bits_per_unit;
        idx /= bits_per_unit;
        if (sz > bits_per_unit) {
            rv.expand((sz-1)/bits_per_unit + 1);
            for (size_t i = 0; i < rv.size; i++) {
                if (shift != 0 && i != 0)
                    rv.ptr[i-1] |= ptr[idx + i] << (bits_per_unit - shift);
                rv.ptr[i] = ptr[idx + i] >> shift; }
            if ((sz %= bits_per_unit))
                rv.ptr[rv.size-1] &= ~(~static_cast<uintptr_t>(1) << (sz-1));
        } else {
            rv.data = ptr[idx] >> shift;
            if (shift != 0 && idx + 1 < size)
                rv.data |= ptr[idx + 1] << (bits_per_unit - shift);
            rv.data &= ~(~static_cast<uintptr_t>(1) << (sz-1)); }
        return rv;
    } else {
        return bitvec((data >> idx) & ~(~static_cast<uintptr_t>(1) << (sz-1))); }
}

int bitvec::ffs(unsigned start) const {
    uintptr_t val = ~static_cast<uintptr_t>(0);
    unsigned idx = start / bits_per_unit;
    val <<= (start % bits_per_unit);
    while (idx < size && !(val &= word(idx))) {
        ++idx;
        val = ~static_cast<uintptr_t>(0); }
    if (idx >= size) return -1;
    unsigned rv = idx * bits_per_unit;
#if defined(__GNUC__) || defined(__clang__)
    rv += builtin_ctz(val);
#else
    while ((val & 0xff) == 0) { rv += 8; val >>= 8; }
    while ((val & 1) == 0) { rv++; val >>= 1; }
#endif
    return rv;
}

unsigned bitvec::ffz(unsigned start) const {
    uintptr_t val = static_cast<uintptr_t>(0);
    unsigned idx = start / bits_per_unit;
    val = ~(~val << (start % bits_per_unit));
    while (!~(val |= word(idx))) {
        ++idx;
        val = 0; }
    unsigned rv = idx * bits_per_unit;
#if defined(__GNUC__) || defined(__clang__)
    rv += builtin_ctz(~val);
#else
    while ((val & 0xff) == 0xff) { rv += 8; val >>= 8; }
    while (val & 1) { rv++; val >>= 1; }
#endif
    return rv;
}

bool bitvec::is_contiguous() const {
    // Empty bitvec is not contiguous
    if (empty())
        return false;
    return max().index() - min().index() + 1 == popcount();
}

bitvec bitvec::rotate_right_helper(size_t start_bit, size_t rotation_idx, size_t end_bit) const {
    assert(start_bit <= rotation_idx && rotation_idx < end_bit);
    bitvec rot_mask(start_bit, end_bit - start_bit);
    bitvec rotation_section = *this & rot_mask;
    int down_shift = rotation_idx - start_bit;
    int up_shift = (end_bit - start_bit) - down_shift;
    bitvec rv;
    rv = (rotation_section >> down_shift) | (rotation_section << up_shift);
    return rv & rot_mask;
}

/**
 * Designed to imitate the std::rotate/std::rotate_copy function for vectors.  Return a bitvec
 * which has the bit at rotation_idx appear at start_bit, and the corresponding data
 * between start_bit and end_bit rotated.  Similar to std::rotate/std::rotate_copy, end_bit is
 * exclusive
 */
void bitvec::rotate_right(size_t start_bit, size_t rotation_idx, size_t end_bit) {
    bitvec rot_section = rotate_right_helper(start_bit, rotation_idx, end_bit);
    clrrange(start_bit, end_bit - start_bit);
    *this |= rot_section;
}

bitvec bitvec::rotate_right_copy(size_t start_bit, size_t rotation_idx, size_t end_bit) const {
    bitvec rot_section = rotate_right_helper(start_bit, rotation_idx, end_bit);
    bitvec rot_mask(start_bit, end_bit - start_bit);
    bitvec rv = rot_section | (*this - rot_mask);
    return rv;
}
