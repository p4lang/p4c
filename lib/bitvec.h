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
#include <gc/gc_cpp.h>
#include <utility>
#include <iostream>

class bitvec {
    size_t              size;
    union {
        uintptr_t       data;
        uintptr_t       *ptr;
    };

 public:
    static constexpr size_t bits_per_unit = CHAR_BIT * sizeof(uintptr_t);

    class bitref {
        friend class bitvec;
        bitvec          &self;
        int             idx;
        bitref(bitvec &s, int i) : self(s), idx(i) {}

     public:
        bitref(const bitref &a) = default;
        bitref(bitref &&a) = default;
        explicit operator bool() const { return self.getbit(idx); }
        operator int() const { return self.getbit(idx) ? 1 : 0; }
        int index() const { return idx; }
        int operator*() const { return idx; }
        bool operator=(bool b) const {
            assert(idx >= 0);
            return b ? self.setbit(idx) : self.clrbit(idx); }
        bool set(bool b = true) {
            assert(idx >= 0);
            bool rv = self.getbit(idx);
            b ? self.setbit(idx) : self.clrbit(idx);
            return rv; }
        bitref &operator++() {
            while ((size_t)++idx < self.size * bitvec::bits_per_unit)
                if (self.getbit(idx)) return *this;
            idx = -1;
            return *this; }
        bitref &operator--() {
            while (--idx >= 0)
                if (self.getbit(idx)) return *this;
            return *this; }
    };
    class const_bitref {
        friend class bitvec;
        const bitvec    &self;
        int             idx;
        const_bitref(const bitvec &s, int i) : self(s), idx(i) {}
     public:
        const_bitref(const const_bitref &a) = default;
        const_bitref(const_bitref &&a) = default;
        explicit operator bool() const { return self.getbit(idx); }
        operator int() const { return self.getbit(idx) ? 1 : 0; }
        int index() const { return idx; }
        int operator*() const { return idx; }
        const_bitref &operator++() {
            while ((size_t)++idx < self.size * bitvec::bits_per_unit)
                if (self.getbit(idx)) return *this;
            idx = -1;
            return *this; }
        const_bitref &operator--() {
            while (--idx >= 0)
                if (self.getbit(idx)) return *this;
            return *this; }
    };

    bitvec() : size(1), data(0) {}
    explicit bitvec(uintptr_t v) : size(1), data(v) {}
    bitvec(size_t lo, size_t cnt) : size(1), data(0) { setrange(lo, cnt); }
    bitvec(const bitvec &a) : size(a.size) {
        if (size > 1) {
            ptr = new(PointerFreeGC) uintptr_t[size];
            memcpy(ptr, a.ptr, size * sizeof(*ptr));
        } else {
            data = a.data; }}
    bitvec(bitvec &&a) : size(a.size), data(a.data) { a.size = 1; }
    bitvec &operator=(const bitvec &a) {
        if (this == &a) return *this;
        if (size > 1) delete [] ptr;
        if ((size = a.size) > 1) {
            ptr = new(PointerFreeGC) uintptr_t[size];
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
            ptr[i] |= (((uintptr_t)1 << (idx%bits_per_unit)) - 1); } }
    void setraw(uintptr_t raw) {
        if (size == 1) {
            data = raw;
        } else {
            ptr[0] = raw;
            for (size_t i = 1; i < size; i++)
                ptr[i] = 0; } }
    void setraw(uintptr_t *raw, size_t sz) {
        if (sz > size) expand(sz);
        if (size == 1) {
            data = raw[0];
        } else {
            for (size_t i = 0; i < sz; i++)
                ptr[i] = raw[i];
            for (size_t i = sz; i < size; i++)
                ptr[i] = 0; } }
    bool clrbit(size_t idx) {
        if (idx >= size * bits_per_unit) return false;
        if (size > 1)
            ptr[idx/bits_per_unit] &= ~((uintptr_t)1 << (idx%bits_per_unit));
        else
            data &= ~((uintptr_t)1 << idx);
        return false; }
    void clrrange(size_t idx, size_t sz) {
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
        if (idx >= size * bits_per_unit) return false;
        if (size > 1)
            return (ptr[idx/bits_per_unit] >> (idx%bits_per_unit)) & 1;
        else
            return (data >> idx) & 1;
        return false; }
    uintptr_t getrange(size_t idx, size_t sz) const {
        assert(sz > 0 && sz <= bits_per_unit);
        if (idx >= size * bits_per_unit) return 0;
        if (size > 1) {
            unsigned shift = idx % bits_per_unit;
            idx /= bits_per_unit;
            uintptr_t rv = ptr[idx] >> shift;
            if (shift != 0 && idx + 1 < size)
                rv |= ptr[idx + 1] << (bits_per_unit - shift);
            return rv & ~(~(uintptr_t)1 << (sz-1));
        } else {
            return (data >> idx) & ~(~(uintptr_t)1 << (sz-1)); }}
    bitvec getslice(size_t idx, size_t sz) const;
    bitref operator[](int idx) { return bitref(*this, idx); }
    bool operator[](int idx) const { return getbit(idx); }
    int ffs(unsigned start = 0) const;
    unsigned ffz(unsigned start = 0) const;
    const_bitref min() const { return const_bitref(*this, ffs()); }
    const_bitref max() const {
        return --const_bitref(*this, size * bits_per_unit); }
    const_bitref begin() const { return min(); }
    const_bitref end() const { return const_bitref(*this, -1); }
    bitref min() { return bitref(*this, ffs()); }
    bitref max() { return --bitref(*this, size * bits_per_unit); }
    bitref begin() { return min(); }
    bitref end() { return bitref(*this, -1); }
    bool empty() const {
        if (size > 1) {
            for (size_t i = 0; i < size; i++)
                if (ptr[i] != 0) return false;
            return true;
        } else { return data == 0; }}
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
    bitvec operator|(const bitvec &a) const {
        bitvec rv(*this); rv |= a; return rv; }
    bitvec operator|(uintptr_t a) const {
        bitvec rv(*this); rv |= a; return rv; }
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
        if (size > 1) {
            if (a.size > 1) {
                size_t i;
                for (i = 0; i < size && i < a.size; i++)
                    if (ptr[i] != a.ptr[i]) return false;
                for (; i < size; i++)
                    if (ptr[i]) return false;
                for (; i < a.size; i++)
                    if (a.ptr[i]) return false;
            } else {
                if (ptr[0] != a.data) return false;
                for (size_t i = 1; i < size; i++)
                    if (ptr[i]) return false; }
        } else if (a.size > 1) {
            if (data != a.ptr[0]) return false;
            for (size_t i = 1; i < a.size; i++)
                if (a.ptr[i]) return false;
        } else {
            return data == a.data; }
        return true; }
    bool operator!=(const bitvec &a) const { return !(*this == a); }
    bool intersects(const bitvec &a) const {
        if (size > 1) {
            if (a.size > 1) {
                for (size_t i = 0; i < size && i < a.size; i++)
                    if (ptr[i] & a.ptr[i]) return true;
                return false;
            } else {
                return (ptr[0] & a.data) != 0; }
        } else if (a.size > 1) {
            return (data & a.ptr[0]) != 0;
        } else {
            return (data & a.data) != 0; }}
    bitvec &operator>>=(size_t count);
    bitvec &operator<<=(size_t count);
    bitvec operator>>(size_t count) const { bitvec rv(*this); rv >>= count; return rv; }
    bitvec operator<<(size_t count) const { bitvec rv(*this); rv <<= count; return rv; }

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
            ptr = new(PointerFreeGC) uintptr_t[newsize];
            memcpy(ptr, old, size * sizeof(*ptr));
            memset(ptr + size, 0, (newsize - size) * sizeof(*ptr));
            delete [] old;
        } else {
            uintptr_t d = data;
            ptr = new(PointerFreeGC) uintptr_t[newsize];
            *ptr = d;
            memset(ptr + size, 0, (newsize - size) * sizeof(*ptr));
        }
        size = newsize;
    }

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
