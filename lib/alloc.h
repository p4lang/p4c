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

#ifndef P4C_LIB_ALLOC_H_
#define P4C_LIB_ALLOC_H_

#include <stdlib.h>
#include <stdexcept>
#include <utility>

template<class T> class Alloc1Dbase {
    int         size;
    T           *data;
 protected:
    explicit Alloc1Dbase(int sz) : size(sz) {
        data = new T[sz];
        for (int i = 0; i < sz; i++) data[i] = T(); }
    Alloc1Dbase(Alloc1Dbase &&a) : size(a.size), data(a.data) { a.data = 0; }
    virtual ~Alloc1Dbase() { delete [] data; }

 public:
    typedef T *iterator;
    typedef T *const_iterator;
    T &operator[](int i) {
        if (i < 0 || i >= size) throw std::out_of_range("Alloc1D");
        return data[i]; }
    const T &operator[](int i) const {
        if (i < 0 || i >= size) throw std::out_of_range("Alloc1D");
        return data[i]; }
    void clear() { for (int i = 0; i < size; i++) data[i] = T(); }
    T *begin() { return data; }
    T *end() { return data + size; }
};

template<class T, int S> class Alloc1D : public Alloc1Dbase<T> {
 public:
    Alloc1D() : Alloc1Dbase<T>(S) {}
    Alloc1Dbase<T> &base() { return *this; }
};

template<class T> class Alloc2Dbase {
    int         nrows, ncols;
    T           *data;
    template<class U>
    class rowref {
        U       *row;
        int     ncols;
        friend class Alloc2Dbase;
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

 protected:
    Alloc2Dbase(int r, int c) : nrows(r), ncols(c) {
        size_t sz = r*c;
        data = new T[sz];
        for (size_t i = 0; i < sz; i++) data[i] = T(); }
    Alloc2Dbase(Alloc2Dbase &&a) : nrows(a.nrows), ncols(a.ncols), data(a.data) { a.data = 0; }
    virtual ~Alloc2Dbase() { delete [] data; }

 public:
    rowref<T> operator[](int i) {
        if (i < 0 || i >= nrows) throw std::out_of_range("Alloc2D");
        return rowref<T>(data+i*ncols, ncols); }
    rowref<const T> operator[](int i) const {
        if (i < 0 || i >= nrows) throw std::out_of_range("Alloc2D");
        return rowref<const T>(data+i*ncols, ncols); }
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
    int rows() const { return nrows; }
    int cols() const { return ncols; }
    void clear() { for (int i = 0; i < nrows*ncols; i++) data[i] = T(); }
};

template<class T, int R, int C> class Alloc2D : public Alloc2Dbase<T> {
 public:
    Alloc2D() : Alloc2Dbase<T>(R, C) {}
    Alloc2Dbase<T> &base() { return *this; }
};

#endif /* P4C_LIB_ALLOC_H_ */
