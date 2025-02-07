/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_ASM_SLIST_H_
#define BF_ASM_SLIST_H_

template <class T>
class slist {
    const slist *next;
    T value;

 public:
    explicit slist(T v) : next(nullptr), value(v) {}
    slist(T v, const slist *n) : next(n), value(v) {}
    typedef T value_type;
    class iterator : public std::iterator<std::forward_iterator_tag, T> {
        friend class slist;
        const slist *ptr;
        iterator() : ptr(nullptr) {}
        explicit iterator(const slist *p) : ptr(p) {}

     public:
        iterator &operator++() {
            ptr = ptr->next;
            return *this;
        }
        bool operator==(const iterator &a) const { return ptr == a.ptr; }
        bool operator!=(const iterator &a) const { return ptr != a.ptr; }
        const T &operator*() const { return ptr->value; }
        const T *operator->() const { return &ptr->value; }
    };
    typedef iterator const_iterator;

    iterator begin() const { return iterator(this); }
    iterator end() const { return iterator(); }
};

#endif /* BF_ASM_SLIST_H_ */
