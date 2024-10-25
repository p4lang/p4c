/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_LIB_DYN_VECTOR_H_
#define BF_P4C_LIB_DYN_VECTOR_H_

#include <vector>

/// An enhanced version of std::vector that auto-expands for
/// operator[].
template <class T, class _Alloc = std::allocator<T>>
class dyn_vector : public std::vector<T, _Alloc> {
 public:
    using std::vector<T, _Alloc>::vector;
    typedef typename std::vector<T, _Alloc>::reference reference;
    typedef typename std::vector<T, _Alloc>::const_reference const_reference;
    typedef typename std::vector<T, _Alloc>::size_type size_type;
    typedef typename std::vector<T>::const_iterator const_iterator;
    reference operator[](size_type n) {
        if (n >= this->size()) this->resize(n + 1);
        return this->at(n);
    }
    const_reference operator[](size_type n) const {
        if (n < this->size()) return this->at(n);
        static const T default_value = T();
        return default_value;
    }
};

#endif /* BF_P4C_LIB_DYN_VECTOR_H_ */
