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

#ifndef P4C_LIB_BITVEC_H_
#define P4C_LIB_BITVEC_H_

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <iostream>
#include "config.h"

#if HAVE_LIBGC
#include <gc/gc_cpp.h>
#define IF_HAVE_LIBGC(X)    X
#else
#define IF_HAVE_LIBGC(X)
#endif /* HAVE_LIBGC */

#ifdef setbit
/* some broken systems define a `setbit' macro in their system header files! */
#undef setbit
#endif
#ifdef clrbit
/* some broken systems define a `clrbit' macro in their system header files! */
#undef clrbit
#endif

#if defined(__GNUC__) || defined(__clang__)
/* use builtin count leading/trailing bits of type-approprite size */
static inline int builtin_ctz(unsigned x) { return __builtin_ctz(x); }
static inline int builtin_ctz(unsigned long x) { return __builtin_ctzl(x); }
static inline int builtin_ctz(unsigned long long x) { return __builtin_ctzll(x); }
static inline int builtin_clz(unsigned x) { return __builtin_clz(x); }
static inline int builtin_clz(unsigned long x) { return __builtin_clzl(x); }
static inline int builtin_clz(unsigned long long x) { return __builtin_clzll(x); }
static inline int builtin_popcount(unsigned x) { return __builtin_popcount(x); }
static inline int builtin_popcount(unsigned long x) { return __builtin_popcountl(x); }
static inline int builtin_popcount(unsigned long long x) { return __builtin_popcountll(x); }
#endif


class bitvec {
    size_t              size;
    union {
        uintptr_t       data;
        uintptr_t       *ptr;
    };
    uintptr_t word(size_t i) const { return i < size ? size > 1 ? ptr[i] : data : 0; }

 public:
    static constexpr size_t bits_per_unit = CHAR_BIT * sizeof(uintptr_t);

 private:
    template<class T> class bitref {
        friend class bitvec;
        T               &self;
        int             idx;
        bitref(T &s, int i) : self(s), idx(i) {}

     public:
        bitref(const bitref &a) = default;
        bitref(bitref &&a) = default;
        operator bool() const { return self.getbit(idx); }
        bool operator==(const bitref &a) const { return &self == &a.self && idx == a.idx; }
        bool operator!=(const bitref &a) const { return &self != &a.self || idx != a.idx; }
        int index() const { return idx; }
        int operator*() const { return idx; }
        bitref &operator++() {
            while ((size_t)++idx < self.size * bitvec::bits_per_unit) {
                if (auto w = self.word(idx/bitvec::bits_per_unit) >> (idx%bitvec::bits_per_unit)) {
#if defined(__GNUC__) || defined(__clang__)
                    idx += builtin_ctz(w);
#else
                    while (!(w & 1)) {
                        ++idx;
                        w >>= 1; }
#endif
                    return *this; }
                idx = (idx / bitvec::bits_per_unit) * bitvec::bits_per_unit
                    + bitvec::bits_per_unit - 1; }
            idx = -1;
            return *this; }
        bitref &operator--() {
            if (idx < 0) idx = self.size * bitvec::bits_per_unit;
            while (--idx >= 0) {
                if (auto w = self.word(idx/bitvec::bits_per_unit)
                           << (bitvec::bits_per_unit - 1 - idx%bitvec::bits_per_unit)) {
#if defined(__GNUC__) || defined(__clang__)
                    idx -= builtin_clz(w);
#else
                    while (!(w >> (bitvec::bits_per_unit - 1))) {
                        --idx;
                        w <<= 1; }
#endif
                    return *this; }
                idx = (idx / bitvec::bits_per_unit) * bitvec::bits_per_unit; }
            return *this; }
    };

 public:
    class nonconst_bitref : public bitref<bitvec> {
        friend class bitvec;
        nonconst_bitref(bitvec &s, int i) : bitref(s, i) {}
     public:
        nonconst_bitref(const bitref<bitvec> &a) : bitref(a) {}  // NOLINT(runtime/explicit)
        nonconst_bitref(const nonconst_bitref &a) = default;
        nonconst_bitref(nonconst_bitref &&a) = default;
        bool operator=(bool b) const {
            assert(idx >= 0);
            return b ? self.setbit(idx) : self.clrbit(idx); }
        bool set(bool b = true) {
            assert(idx >= 0);
            bool rv = self.getbit(idx);
            b ? self.setbit(idx) : self.clrbit(idx);
            return rv; }
    };
    typedef nonconst_bitref     iterator;

    class const_bitref : public bitref<const bitvec> {
        friend class bitvec;
        const_bitref(const bitvec &s, int i) : bitref(s, i) {}
     public:
        const_bitref(const bitref<const bitvec> &a) : bitref(a) {}  // NOLINT(runtime/explicit)
        const_bitref(const const_bitref &a) = default;
        const_bitref(const_bitref &&a) = default;
    };
    typedef const_bitref        const_iterator;

    bitvec() : size(1), data(0) {}
    explicit bitvec(uintptr_t v) : size(1), data(v) {}
    template<typename T, typename = typename
        std::enable_if<std::is_integral<T>::value && (sizeof(T) > sizeof(uintptr_t))>::type>
    explicit bitvec(T v) : size(1), data(v) {
        if (v != data) {
            size = sizeof(v)/sizeof(data);
            ptr = new IF_HAVE_LIBGC((PointerFreeGC)) uintptr_t[size];
            for (unsigned i = 0; i < size; ++i) {
                ptr[i] = v;
                v >>= bits_per_unit; } } }
    bitvec(size_t lo, size_t cnt) : size(1), data(0) { setrange(lo, cnt); }
    bitvec(const bitvec &a) : size(a.size) {
        if (size > 1) {
            ptr = new IF_HAVE_LIBGC((PointerFreeGC)) uintptr_t[size];
            memcpy(ptr, a.ptr, size * sizeof(*ptr));
        } else {
            data = a.data; }}
    bitvec(bitvec &&a) : size(a.size), data(a.data) { a.size = 1; }
    bitvec &operator=(const bitvec &a) {
        if (this == &a) return *this;
        if (size > 1) delete [] ptr;
        if ((size = a.size) > 1) {
            ptr = new IF_HAVE_LIBGC((PointerFreeGC)) uintptr_t[size];
            memcpy(ptr, a.ptr, size * sizeof(*ptr));
        } else {
            data = a.data; }
        return *this; }
    bitvec &operator=(bitvec &&a) {
        std::swap(size, a.size); std::swap(data, a.data);
        return *this; }
    ~bitvec() { if (size > 1) delete [] ptr; }

    void clear() {
        if (size > 1) memset(ptr, 0, size * sizeof(*ptr));
        else data = 0; }  // NOLINT(whitespace/newline)
    bool setbit(size_t idx) {
        if (idx >= size * bits_per_unit) expand(1 + idx/bits_per_unit);
        if (size > 1)
            ptr[idx/bits_per_unit] |= (uintptr_t)1 << (idx%bits_per_unit);
        else
            data |= (uintptr_t)1 << idx;
        return true; }
    void setrange(size_t idx, size_t sz) {
        if (sz == 0) return;
        if (idx+sz > size * bits_per_unit) expand(1 + (idx+sz-1)/bits_per_unit);
        if (size == 1) {
            data |= ~(~(uintptr_t)1 << (sz-1)) << idx;
        } else if (idx/bits_per_unit == (idx+sz-1)/bits_per_unit) {
            ptr[idx/bits_per_unit] |=
                ~(~(uintptr_t)1 << (sz-1)) << (idx%bits_per_unit);
        } else {
            size_t i = idx/bits_per_unit;
            ptr[i] |= ~(uintptr_t)0 << (idx%bits_per_unit);
            idx += sz;
            while (++i < idx/bits_per_unit) {
                ptr[i] = ~(uintptr_t)0; }
            if (i < size)
                ptr[i] |= (((uintptr_t)1 << (idx%bits_per_unit)) - 1); } }
    void setraw(uintptr_t raw) {
        if (size == 1) {
            data = raw;
        } else {
            ptr[0] = raw;
            for (size_t i = 1; i < size; i++)
                ptr[i] = 0; } }
    template<typename T, typename = typename
        std::enable_if<std::is_integral<T>::value && (sizeof(T) > sizeof(uintptr_t))>::type>
    void setraw(T raw) {
        if (sizeof(T)/sizeof(uintptr_t) > size) expand(sizeof(T)/sizeof(uintptr_t));
        for (size_t i = 0; i < size; i++) {
            ptr[i] = raw;
            raw >>= bits_per_unit; } }
    void setraw(uintptr_t *raw, size_t sz) {
        if (sz > size) expand(sz);
        if (size == 1) {
            data = raw[0];
        } else {
            for (size_t i = 0; i < sz; i++)
                ptr[i] = raw[i];
            for (size_t i = sz; i < size; i++)
                ptr[i] = 0; } }
    template<typename T, typename = typename
        std::enable_if<std::is_integral<T>::value && (sizeof(T) > sizeof(uintptr_t))>::type>
    void setraw(T *raw, size_t sz) {
        constexpr size_t m = sizeof(T)/sizeof(uintptr_t);
        if (m * sz > size) expand(m * sz);
        size_t i = 0;
        for (; i < sz*m; ++i)
            ptr[i] = raw[i/m] >> ((i%m) * bits_per_unit);
        for (; i < size; ++i)
            ptr[i] = 0; }
    bool clrbit(size_t idx) {
        if (idx >= size * bits_per_unit) return false;
        if (size > 1)
            ptr[idx/bits_per_unit] &= ~((uintptr_t)1 << (idx%bits_per_unit));
        else
            data &= ~((uintptr_t)1 << idx);
        return false; }
    void clrrange(size_t idx, size_t sz) {
        if (sz == 0) return;
        if (size < sz/bits_per_unit)  // To avoid sz + idx overflow
            sz = size * bits_per_unit;
        if (idx >= size * bits_per_unit) return;
        if (size == 1) {
            if (idx + sz < bits_per_unit)
                data &= ~(~(~(uintptr_t)1 << (sz-1)) << idx);
            else
                data &= ~(~(uintptr_t)0 << idx);
        } else if (idx/bits_per_unit == (idx+sz-1)/bits_per_unit) {
            ptr[idx/bits_per_unit] &=
                ~(~(~(uintptr_t)1 << (sz-1)) << (idx%bits_per_unit));
        } else {
            size_t i = idx/bits_per_unit;
            ptr[i] &= ~(~(uintptr_t)0 << (idx%bits_per_unit));
            idx += sz;
            while (++i < idx/bits_per_unit && i < size) {
                ptr[i] = 0; }
            if (i < size)
                ptr[i] &= ~(((uintptr_t)1 << (idx%bits_per_unit)) - 1); } }
    bool getbit(size_t idx) const {
        return (word(idx/bits_per_unit) >> (idx%bits_per_unit)) & 1; }
    uintmax_t getrange(size_t idx, size_t sz) const {
        assert(sz > 0 && sz <= CHAR_BIT * sizeof(uintmax_t));
        if (idx >= size * bits_per_unit) return 0;
        if (size > 1) {
            unsigned shift = idx % bits_per_unit;
            idx /= bits_per_unit;
            uintmax_t rv = ptr[idx] >> shift;
            shift = bits_per_unit - shift;
            while (shift < sz) {
                if (++idx >= size) break;
                rv |= (uintmax_t)ptr[idx] << shift;
                shift += bits_per_unit; }
            return rv & ~(~(uintmax_t)1 << (sz-1));
        } else {
            return (data >> idx) & ~(~(uintmax_t)1 << (sz-1)); } }
    void putrange(size_t idx, size_t sz, uintmax_t v) {
        assert(sz > 0 && sz <= CHAR_BIT * sizeof(uintmax_t));
        uintptr_t mask = ~(uintmax_t)0 >> (CHAR_BIT * sizeof(uintmax_t) - sz);
        v &= mask;
        if (idx+sz > size * bits_per_unit) expand(1 + (idx+sz-1)/bits_per_unit);
        if (size == 1) {
            data &= ~(mask << idx);
            data |= v << idx;
        } else {
            unsigned shift = idx % bits_per_unit;
            idx /= bits_per_unit;
            ptr[idx] &= ~(mask << shift);
            ptr[idx] |= v << shift;
            shift = bits_per_unit - shift;
            while (shift < sz) {
                assert(idx+1 < size);
                ptr[++idx] &= ~(mask >> shift);
                ptr[idx] |= v >> shift;
                shift += bits_per_unit; } } }
    bitvec getslice(size_t idx, size_t sz) const;
    nonconst_bitref operator[](int idx) { return nonconst_bitref(*this, idx); }
    bool operator[](int idx) const { return getbit(idx); }
    int ffs(unsigned start = 0) const;
    unsigned ffz(unsigned start = 0) const;
    const_bitref min() const { return const_bitref(*this, ffs()); }
    const_bitref max() const {
        return --const_bitref(*this, size * bits_per_unit); }
    const_bitref begin() const { return min(); }
    const_bitref end() const { return const_bitref(*this, -1); }
    nonconst_bitref min() { return nonconst_bitref(*this, ffs()); }
    nonconst_bitref max() { return --nonconst_bitref(*this, size * bits_per_unit); }
    nonconst_bitref begin() { return min(); }
    nonconst_bitref end() { return nonconst_bitref(*this, -1); }
    bool empty() const {
        for (size_t i = 0; i < size; i++)
            if (word(i) != 0) return false;
        return true; }
    explicit operator bool() const { return !empty(); }
    bool operator&=(const bitvec &a) {
        bool rv = false;
        if (size > 1) {
            if (a.size > 1) {
                for (size_t i = 0; i < size && i < a.size; i++) {
                    rv |= ((ptr[i] & a.ptr[i]) != ptr[i]);
                    ptr[i] &= a.ptr[i]; }
            } else {
                rv |= ((*ptr & a.data) != *ptr);
                *ptr &= a.data; }
            if (size > a.size) {
                if (!rv) {
                    for (size_t i = a.size; i < size; i++)
                        if (ptr[i]) { rv = true; break; }}
                memset(ptr + a.size, 0, (size-a.size) * sizeof(*ptr)); }
        } else if (a.size > 1) {
            rv |= ((data & a.ptr[0]) != data);
            data &= a.ptr[0];
        } else {
            rv |= ((data & a.data) != data);
            data &= a.data; }
        return rv; }
    bitvec operator&(const bitvec &a) const {
        if (size <= a.size) {
            bitvec rv(*this); rv &= a; return rv;
        } else {
            bitvec rv(a); rv &= *this; return rv; } }
    bool operator|=(const bitvec &a) {
        bool rv = false;
        if (size < a.size) expand(a.size);
        if (size > 1) {
            if (a.size > 1) {
                for (size_t i = 0; i < a.size; i++) {
                    rv |= ((ptr[i] | a.ptr[i]) != ptr[i]);
                    ptr[i] |= a.ptr[i]; }
            } else {
                rv |= ((*ptr | a.data) != *ptr);
                *ptr |= a.data; }
        } else {
            rv |= ((data | a.data) != data);
            data |= a.data; }
        return rv; }
    bool operator|=(uintptr_t a) {
        bool rv = false;
        auto t = size > 1 ? ptr : &data;
        rv |= ((*t | a) != *t);
        *t |= a;
        return rv; }
    template<typename T, typename = typename
             std::enable_if<std::is_integral<T>::value && (sizeof(T) > sizeof(uintptr_t))>::type>
    bool operator|=(T a) { return (*this) |= bitvec(a); }
    bitvec operator|(const bitvec &a) const {
        bitvec rv(*this); rv |= a; return rv; }
    bitvec operator|(uintptr_t a) const {
        bitvec rv(*this); rv |= a; return rv; }
    template<typename T, typename = typename
             std::enable_if<std::is_integral<T>::value && (sizeof(T) > sizeof(uintptr_t))>::type>
    bitvec operator|(T a) { bitvec rv(*this); rv |= bitvec(a); return rv; }
    bitvec &operator^=(const bitvec &a) {
        if (size < a.size) expand(a.size);
        if (size > 1) {
            if (a.size > 1) {
                for (size_t i = 0; i < a.size; i++) ptr[i] ^= a.ptr[i];
            } else {
                *ptr ^= a.data; }
        } else {
            data ^= a.data; }
        return *this; }
    bitvec operator^(const bitvec &a) const {
        bitvec rv(*this); rv ^= a; return rv; }
    bool operator-=(const bitvec &a) {
        bool rv = false;
        if (size > 1) {
            if (a.size > 1) {
                for (size_t i = 0; i < size && i < a.size; i++) {
                    rv |= ((ptr[i] & ~a.ptr[i]) != ptr[i]);
                    ptr[i] &= ~a.ptr[i]; }
            } else {
                rv |= ((*ptr & ~a.data) != *ptr);
                *ptr &= ~a.data; }
        } else if (a.size > 1) {
            rv |= ((data & ~a.ptr[0]) != data);
            data &= ~a.ptr[0];
        } else {
            rv |= ((data & ~a.data) != data);
            data &= ~a.data; }
        return rv; }
    bitvec operator-(const bitvec &a) const {
        bitvec rv(*this); rv -= a; return rv; }
    bool operator==(const bitvec &a) const {
        for (size_t i = 0; i < size || i < a.size; i++)
            if (word(i) != a.word(i)) return false;
        return true; }
    bool operator!=(const bitvec &a) const { return !(*this == a); }
    bool operator<(const bitvec &a) const {
        size_t i = std::max(size, a.size);
        while (i--) {
            if (word(i) < a.word(i)) return true;
            if (word(i) > a.word(i)) return false; }
        return false; }
    bool operator>(const bitvec &a) const { return a < *this; }
    bool operator>=(const bitvec &a) const { return !(*this < a); }
    bool operator<=(const bitvec &a) const { return !(a < *this); }
    bool intersects(const bitvec &a) const {
        for (size_t i = 0; i < size && i < a.size; i++)
            if (word(i) & a.word(i)) return true;
        return false; }
    bool contains(const bitvec &a) const {  // is 'a' a subset or equal to 'this'?
        for (size_t i = 0; i < size && i < a.size; i++)
            if ((word(i) & a.word(i)) != a.word(i)) return false;
        for (size_t i = size; i < a.size; i++)
            if (a.word(i)) return false;
        return true; }
    bitvec &operator>>=(size_t count);
    bitvec &operator<<=(size_t count);
    bitvec operator>>(size_t count) const { bitvec rv(*this); rv >>= count; return rv; }
    bitvec operator<<(size_t count) const { bitvec rv(*this); rv <<= count; return rv; }
    void rotate_right(size_t start_bit, size_t rotation_idx, size_t end_bit);
    bitvec rotate_right_copy(size_t start_bit, size_t rotation_idx, size_t end_bit) const;
    int popcount() const {
        int rv = 0;
        for (size_t i = 0; i < size; i++)
#if defined(__GNUC__) || defined(__clang__)
            rv += builtin_popcount(word(i));
#else
            for (auto v = word(i); v; v &= v-1)
                ++rv;
#endif
        return rv; }
    bool is_contiguous() const;

 private:
    void expand(size_t newsize) {
        assert(newsize > size);
        if (size_t m = newsize>>3) {
            /* round up newsize to be at most 7*2**k, to avoid reallocing too much */
            m |= m >> 1;
            m |= m >> 2;
            m |= m >> 4;
            m |= m >> 8;
            m |= m >> 16;
            newsize = (newsize + m) & ~m; }
        if (size > 1) {
            uintptr_t *old = ptr;
            ptr = new IF_HAVE_LIBGC((PointerFreeGC)) uintptr_t[newsize];
            memcpy(ptr, old, size * sizeof(*ptr));
            memset(ptr + size, 0, (newsize - size) * sizeof(*ptr));
            delete [] old;
        } else {
            uintptr_t d = data;
            ptr = new IF_HAVE_LIBGC((PointerFreeGC)) uintptr_t[newsize];
            *ptr = d;
            memset(ptr + size, 0, (newsize - size) * sizeof(*ptr));
        }
        size = newsize;
    }

    bitvec rotate_right_helper(size_t start_bit, size_t rotation_idx, size_t end_bit) const;

 public:
    friend std::ostream &operator<<(std::ostream &, const bitvec &);
};

inline bitvec operator|(bitvec &&a, const bitvec &b) {
    bitvec rv(std::move(a)); rv |= b; return rv; }
inline bitvec operator&(bitvec &&a, const bitvec &b) {
    bitvec rv(std::move(a)); rv &= b; return rv; }
inline bitvec operator^(bitvec &&a, const bitvec &b) {
    bitvec rv(std::move(a)); rv ^= b; return rv; }
inline bitvec operator-(bitvec &&a, const bitvec &b) {
    bitvec rv(std::move(a)); rv -= b; return rv; }

#endif  // P4C_LIB_BITVEC_H_
