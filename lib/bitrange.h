#ifndef _LIB_BITRANGE_H_
#define _LIB_BITRANGE_H_

#include "bitvec.h"

/* iterate over ranges of contiguous bits in a bitvector */
class bitranges {
    bitvec tmp;
    const bitvec &bits;
    struct iter {
        bool valid = true;
        bitvec::const_bitref    ptr;
        std::pair<int, int>     range;

        iter &operator++() {
            if (ptr) {
                range.first = range.second = ptr.index();
                while(++ptr && range.second+1 == ptr.index()) ++range.second;
            } else {
                valid = false; }
            return *this; }
        std::pair<int, int> operator*() { return range; }
        bool operator==(iter &a) const { return valid == a.valid && ptr == a.ptr; }
        bool operator!=(iter &a) const { return !(*this == a); }
        iter(bitvec::const_bitref p) : ptr(p) { ++*this; }
    };

public:
    bitranges(const bitvec &b) : bits(b) {}
    bitranges(uintptr_t b) : tmp(b), bits(tmp) {}
    iter begin() const { return iter(bits.begin()); }
    iter end() const { return iter(bits.end()); }
};


#endif /* _LIB_BITRANGE_H_ */
