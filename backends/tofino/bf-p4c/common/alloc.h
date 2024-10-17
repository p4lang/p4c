/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_COMMON_ALLOC_H_
#define EXTENSIONS_BF_P4C_COMMON_ALLOC_H_

#include <stdlib.h>
#include <stdexcept>
#include <utility>

namespace BFN {

template<class T> class Alloc1Dbase {
    int         size_;
    T           *data;
    Alloc1Dbase() = delete;
    Alloc1Dbase(const Alloc1Dbase &) = delete;
    Alloc1Dbase &operator=(const Alloc1Dbase &) = delete;
    Alloc1Dbase &operator=(Alloc1Dbase &&) = delete;

 public:
    explicit Alloc1Dbase(int sz) : size_(sz) {
        data = sz ? new T[sz] {} : nullptr; }
    Alloc1Dbase(Alloc1Dbase &&a) noexcept : size_(a.size_), data(a.data) { a.data = 0; }
    virtual ~Alloc1Dbase() { delete [] data; }

    typedef T *iterator;
    typedef T *const_iterator;
    T &operator[](int i) {
        if (i < 0 || i >= size_) throw std::out_of_range("Alloc1D");
        return data[i]; }
    const T &operator[](int i) const {
        if (i < 0 || i >= size_) throw std::out_of_range("Alloc1D");
        return data[i]; }
    bool operator==(const Alloc1Dbase<T> &t) const {
        return std::equal(data, data + size_, t.data, t.data + t.size_); }
    bool operator!=(const Alloc1Dbase<T> &t) const { return !(*this==t); }

    int size() const { return size_; }
    void clear() { std::fill(data, data + size_, T()); }
    T *begin() { return data; }
    T *end() { return data + size_; }
};

template<class T, int S> class Alloc1D : public Alloc1Dbase<T> {
 public:
    Alloc1D() : Alloc1Dbase<T>(S) {}
    Alloc1Dbase<T> &base() { return *this; }
    bool operator!=(const Alloc1D<T,S> &t) const {
        return Alloc1Dbase<T>::operator!=(t); }
};

template<class T> class Alloc3Dbase;

template<class T> class Alloc2Dbase {
    int         nrows, ncols;
    T           *data;
    template<class U>
    class rowref {
        U       *row;
        int     ncols;
        friend class Alloc2Dbase;
        friend class Alloc3Dbase<U>;
        rowref(U *r, int c) : row(r), ncols(c) {}
     public:
        typedef U *iterator;
        typedef const U *const_iterator;
        U &operator[](int i) const {
            if (i < 0 || i >= ncols) throw std::out_of_range("Alloc2D");
            return row[i]; }
        U *begin() const { return row; }
        U *end() const { return row + ncols; }
    };
    Alloc2Dbase() = delete;
    Alloc2Dbase(const Alloc2Dbase &) = delete;
    Alloc2Dbase &operator=(const Alloc2Dbase &) = delete;
    Alloc2Dbase &operator=(Alloc2Dbase &&) = delete;
    friend class Alloc3Dbase<T>;

 public:
    Alloc2Dbase(int r, int c) : nrows(r), ncols(c) {
        size_t sz = r*c;
        data = sz ? new T[sz] {} : nullptr; }
    Alloc2Dbase(Alloc2Dbase &&a) noexcept : nrows(a.nrows), ncols(a.ncols), data(a.data) {
        a.data = 0; }
    virtual ~Alloc2Dbase() { delete [] data; }

    rowref<T> operator[](int i) {
        if (i < 0 || i >= nrows) throw std::out_of_range("Alloc2D");
        return { data+i*ncols, ncols }; }
    rowref<const T> operator[](int i) const {
        if (i < 0 || i >= nrows) throw std::out_of_range("Alloc2D");
        return { data+i*ncols, ncols }; }
    T &at(int i, int j) {
        if (i < 0 || i >= nrows || j < 0 || j >= ncols)
            throw std::out_of_range("Alloc2D");
        return data[i*ncols + j]; }
    const T &at(int i, int j) const {
        if (i < 0 || i >= nrows || j < 0 || j >= ncols)
            throw std::out_of_range("Alloc2D");
        return data[i*ncols + j]; }
    T &operator[](std::pair<int, int> i) {
        if (i.first < 0 || i.first >= nrows ||
            i.second < 0 || i.second >= ncols)
            throw std::out_of_range("Alloc2D");
        return data[i.first*ncols + i.second]; }
    const T &operator[](std::pair<int, int> i) const {
        if (i.first < 0 || i.first >= nrows ||
            i.second < 0 || i.second >= ncols)
            throw std::out_of_range("Alloc2D");
        return data[i.first*ncols + i.second]; }
    bool operator==(const Alloc2Dbase<T> &t) const {
        int sz = nrows*ncols;
        if (nrows != t.nrows || ncols != t.ncols)
            return false;
        return std::equal(data, data + sz, t.data); }
    bool operator!=(const Alloc2Dbase<T> &t) const { return !(*this==t); }

    int rows() const { return nrows; }
    int cols() const { return ncols; }
    void clear() { std::fill(data, data + nrows*ncols, T()); }
};

template<class T, int R, int C> class Alloc2D : public Alloc2Dbase<T> {
 public:
    Alloc2D() : Alloc2Dbase<T>(R, C) {}
    Alloc2Dbase<T> &base() { return *this; }
};

template<class T> class Alloc3Dbase {
    int         nmats, nrows, ncols;
    T           *data;
    template<class U>
    class matref {
        U       *matrix;
        int     nrows, ncols;
        friend class Alloc3Dbase;
     public:
        typename Alloc2Dbase<T>::template rowref<U> operator[](int i) const {
            if (i < 0 || i >= nrows) throw std::out_of_range("Alloc3D");
            return { matrix + i*ncols, ncols }; }
        U &operator[](std::pair<int, int> i) const {
            if (i.first < 0 || i.first >= nrows ||
                i.second < 0 || i.second >= ncols)
                throw std::out_of_range("Alloc3D");
            return matrix[i.first*ncols + i.second]; }
    };
    Alloc3Dbase() = delete;
    Alloc3Dbase(const Alloc3Dbase &) = delete;
    Alloc3Dbase &operator=(const Alloc3Dbase &) = delete;
    Alloc3Dbase &operator=(Alloc3Dbase &&) = delete;

 public:
    Alloc3Dbase(int m, int r, int c) : nmats(m), nrows(r), ncols(c) {
        size_t sz = m*r*c;
        data = sz ? new T[sz] {} : nullptr; }
    Alloc3Dbase(Alloc3Dbase &&a) noexcept
    : nmats(a.nmats), nrows(a.nrows), ncols(a.ncols), data(a.data) {
        a.data = 0; }
    virtual ~Alloc3Dbase() { delete [] data; }

    matref<T> operator[](int i) {
        if (i < 0 || i >= nmats) throw std::out_of_range("Alloc3D");
        return { data+i*nrows*ncols, nrows, ncols }; }
    matref<const T> operator[](int i) const {
        if (i < 0 || i >= nmats) throw std::out_of_range("Alloc3D");
        return { data+i*nrows*ncols, nrows, ncols }; }
    T &at(int i, int j, int k) {
        if (i < 0 || i >= nmats || j < 0 || j >= nrows || k < 0 || k >= ncols)
            throw std::out_of_range("Alloc3D");
        return data[i*nrows*ncols + j*ncols + k]; }
    const T &at(int i, int j, int k) const {
        if (i < 0 || i >= nmats || j < 0 || j >= nrows || k < 0 || k >= ncols)
            throw std::out_of_range("Alloc3D");
        return data[i*nrows*ncols + j*ncols + k]; }
    T &operator[](std::tuple<int, int, int> i) {
        if (std::get<0>(i) < 0 || std::get<0>(i) >= nmats ||
            std::get<1>(i) < 0 || std::get<1>(i) >= nrows ||
            std::get<2>(i) < 0 || std::get<2>(i) >= ncols)
            throw std::out_of_range("Alloc3D");
        return data[std::get<0>(i)*nrows*ncols + std::get<1>(i)*ncols + std::get<2>(i)]; }
    const T &operator[](std::tuple<int, int, int> i) const {
        if (std::get<0>(i) < 0 || std::get<0>(i) >= nmats ||
            std::get<1>(i) < 0 || std::get<1>(i) >= nrows ||
            std::get<2>(i) < 0 || std::get<2>(i) >= ncols)
            throw std::out_of_range("Alloc3D");
        return data[std::get<0>(i)*nrows*ncols + std::get<1>(i)*ncols + std::get<2>(i)]; }
    bool operator==(const Alloc3Dbase<T> &t) const {
        int sz = nmats*nrows*ncols;
        if (nmats != t.nmats || nrows != t.nrows || ncols != t.ncols)
            return false;
        return std::equal(data, data + sz, t.data); }
    bool operator!=(const Alloc3Dbase<T> &t) const { return !(*this==t); }

    int matrixes() const { return nmats; }
    int rows() const { return nrows; }
    int cols() const { return ncols; }
    void clear() { std::fill(data, data + nmats*nrows*ncols, T()); }
};

template<class T, int B, int R, int C> class Alloc3D : public Alloc3Dbase<T> {
 public:
    Alloc3D() : Alloc3Dbase<T>(B, R, C) {}
    Alloc3Dbase<T> &base() { return *this; }
};

}  // namespace BFN

#endif /* EXTENSIONS_BF_P4C_COMMON_ALLOC_H_ */
