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

#ifndef IR_STD_H_
#define IR_STD_H_
#include <array>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::const_pointer_cast;
using std::map;
using std::set;
using std::unordered_map;
using std::unordered_set;
// using std::vector;
// using std::array;

template<class T, class _Alloc = std::allocator<T>>
class vector : public std::vector<T, _Alloc> {
 public:
    using std::vector<T, _Alloc>::vector;
    typedef typename std::vector<T, _Alloc>::reference reference;
    typedef typename std::vector<T, _Alloc>::const_reference const_reference;
    typedef typename std::vector<T, _Alloc>::size_type size_type;
    typedef typename std::vector<T>::const_iterator const_iterator;
    reference operator[](size_type n) { return this->at(n); }
    const_reference operator[](size_type n) const { return this->at(n); }
};

template<class T, size_t N>
class array : public std::array<T, N> {
 public:
    using std::array<T, N>::array;
    typedef typename std::array<T, N>::reference reference;
    typedef typename std::array<T, N>::const_reference const_reference;
    typedef typename std::array<T, N>::size_type size_type;
    reference operator[](size_type n) { return this->at(n); }
    const_reference operator[](size_type n) const { return this->at(n); }
};

#endif /* IR_STD_H_ */
