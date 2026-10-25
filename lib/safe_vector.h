/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_SAFE_VECTOR_H_
#define LIB_SAFE_VECTOR_H_

#include <vector>

namespace P4 {

/// An enhanced version of std::vector that performs bounds checking for
/// operator[].
template <class T, class Alloc = std::allocator<T>>
class safe_vector : public std::vector<T, Alloc> {
 public:
    using std::vector<T, Alloc>::vector;
    typedef typename std::vector<T, Alloc>::reference reference;
    typedef typename std::vector<T, Alloc>::const_reference const_reference;
    typedef typename std::vector<T, Alloc>::size_type size_type;
    typedef typename std::vector<T>::const_iterator const_iterator;
    reference operator[](size_type n) { return this->at(n); }
    const_reference operator[](size_type n) const { return this->at(n); }
};

}  // namespace P4

#endif /* LIB_SAFE_VECTOR_H_ */
