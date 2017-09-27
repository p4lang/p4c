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

#ifndef P4C_LIB_SAFE_VECTOR_H_
#define P4C_LIB_SAFE_VECTOR_H_

#include <vector>

/// An enhanced version of std::vector that performs bounds checking for
/// operator[].
template<class T, class _Alloc = std::allocator<T>>
class safe_vector : public std::vector<T, _Alloc> {
 public:
    using std::vector<T, _Alloc>::vector;
    typedef typename std::vector<T, _Alloc>::reference reference;
    typedef typename std::vector<T, _Alloc>::const_reference const_reference;
    typedef typename std::vector<T, _Alloc>::size_type size_type;
    typedef typename std::vector<T>::const_iterator const_iterator;
    reference operator[](size_type n) { return this->at(n); }
    const_reference operator[](size_type n) const { return this->at(n); }
};

#endif /* P4C_LIB_SAFE_VECTOR_H_ */
