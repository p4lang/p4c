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

#ifndef P4C_LIB_SYMBITMATRIX_H_
#define P4C_LIB_SYMBITMATRIX_H_

#include "bitvec.h"

/* A symmetric bit matrix, held in a bit vector.  We only store a triangular submatrix which
 * is used for both halves, so modifying one bit modifies both sides, keeping the matrix
 * always symmetric.  Iterating over the matrix only iterates over the lower triangle */
class SymBitMatrix : private bitvec {
 public:
    nonconst_bitref operator()(unsigned r, unsigned c) {
        if (r < c) std::swap(r, c);
        return bitvec::operator[]((r*r+r)/2 + c); }
    bool operator()(unsigned r, unsigned c) const {
        if (r < c) std::swap(r, c);
        return bitvec::operator[]((r*r+r)/2 + c); }
    unsigned size() const {
        if (empty()) return 0;
        unsigned m = *max();
        unsigned r = 1;
        while ((r*r+r)/2 <= m) r++;
        return r; }
    using bitvec::clear;
    using bitvec::empty;
    using bitvec::operator bool;

 private:
    using bitvec::operator|=;
    template<class T> class rowref {
        friend class SymBitMatrix;
        T               &self;
        unsigned        row;
        rowref(T &s, unsigned r) : self(s), row(r) {}

     public:
        rowref(const rowref &) = default;
        rowref(rowref &&) = default;
        explicit operator bool() const {
            if (row < bits_per_unit) {
                if (self.getrange((row*row+row)/2, row+1) != 0) return true;
            } else {
                if (self.getslice((row*row+row)/2, row+1)) return true; }
            for (auto c = self.size()-1; c > row; --c)
                if (self(row, c)) return true;
            return false; }
        operator bitvec() const {
            auto rv = self.getslice((row*row+row)/2, row+1);
            for (auto c = self.size()-1; c > row; --c)
                if (self(row, c)) rv[c] = 1;
            return rv; }
    };
    class nonconst_rowref : public rowref<SymBitMatrix> {
     public:
        friend class SymBitMatrix;
        using rowref<SymBitMatrix>::rowref;
        void operator|=(bitvec a) const {
            for (size_t v : a) {
                self(row, v) = 1; } }
        nonconst_bitref operator[](unsigned col) const { return self(row, col); }
    };
    class const_rowref : public rowref<const SymBitMatrix> {
     public:
        friend class SymBitMatrix;
        using rowref<const SymBitMatrix>::rowref;
        bool operator[](unsigned col) const { return self(row, col); }
    };

 public:
    nonconst_rowref operator[](unsigned r) { return nonconst_rowref(*this, r); }
    const_rowref operator[](unsigned r) const { return const_rowref(*this, r); }

    bool operator==(const SymBitMatrix &a) const { return bitvec::operator==(a); }
    bool operator!=(const SymBitMatrix &a) const { return bitvec::operator!=(a); }
    bool operator|=(const SymBitMatrix &a) { return bitvec::operator|=(a); }
};

#endif /* P4C_LIB_SYMBITMATRIX_H_ */
