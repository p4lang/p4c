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
                while (++ptr && range.second+1 == ptr.index()) ++range.second;
            } else {
                valid = false; }
            return *this; }
        std::pair<int, int> operator*() { return range; }
        bool operator==(iter &a) const { return valid == a.valid && ptr == a.ptr; }
        bool operator!=(iter &a) const { return !(*this == a); }
        explicit iter(bitvec::const_bitref p) : ptr(p) { ++*this; }
    };

 public:
    explicit bitranges(const bitvec &b) : bits(b) {}
    explicit bitranges(bitvec &&b) : tmp(b), bits(tmp) {}
    explicit bitranges(uintptr_t b) : tmp(b), bits(tmp) {}
    iter begin() const { return iter(bits.begin()); }
    iter end() const { return iter(bits.end()); }
};


#endif /* _LIB_BITRANGE_H_ */
