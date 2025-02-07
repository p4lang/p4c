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

#ifndef BF_ASM_ALIAS_ARRAY_H_
#define BF_ASM_ALIAS_ARRAY_H_

#include <stdlib.h>

#include "bfas.h"  // for BUG_CHECK

template <size_t S, typename T>
class alias_array;

template <typename T>
class alias_array_base {
 protected:
    class iterator {
        T **ptr;

     public:
        explicit iterator(T **p) : ptr(p) {}
        iterator &operator++() {
            ++ptr;
            return *this;
        }
        iterator &operator--() {
            --ptr;
            return *this;
        }
        iterator &operator++(int) {
            auto copy = *this;
            ++ptr;
            return copy;
        }
        iterator &operator--(int) {
            auto copy = *this;
            --ptr;
            return copy;
        }
        bool operator==(const iterator &i) const { return ptr == i.ptr; }
        bool operator!=(const iterator &i) const { return ptr != i.ptr; }
        T &operator*() const { return **ptr; }
        T *operator->() const { return *ptr; }
    };

 public:
    virtual T &operator[](size_t) = 0;
    virtual const T &operator[](size_t) const = 0;
    virtual size_t size() const = 0;
    virtual iterator begin() = 0;
    virtual iterator end() = 0;
    virtual bool modified() const = 0;
    virtual void set_modified(bool v = true) = 0;
    virtual bool disabled() const = 0;
    virtual bool disable() = 0;
    virtual bool disable_if_zero() = 0;
    virtual void enable() = 0;
};

template <size_t S, typename T>
class alias_array : public alias_array_base<T> {
    T *data[S];
    using typename alias_array_base<T>::iterator;

 public:
    alias_array(const std::initializer_list<T *> &v) {
        auto it = v.begin();
        for (auto &e : data) {
            BUG_CHECK(it != v.end(), "Not enough initializers for alias array");
            e = *it++;
        }
        BUG_CHECK(it == v.end(), "Too many initializers for alias array");
    }
    T &operator[](size_t idx) {
        BUG_CHECK(idx < S, "alias array index %zd out of bounds %zd", idx, S);
        return *data[idx];
    }
    const T &operator[](size_t idx) const {
        BUG_CHECK(idx < S, "alias array index %zd out of bounds %zd", idx, S);
        return *data[idx];
    }
    size_t size() const { return S; }
    iterator begin() { return iterator(data); }
    iterator end() { return iterator(data + S); }
    bool modified() const {
        for (size_t i = 0; i < S; i++)
            if (data[i]->modified()) return true;
        return false;
    }
    void set_modified(bool v = true) {
        for (size_t i = 0; i < S; i++) data[i]->set_modified(v);
    }
    bool disabled() const {
        bool rv = true;
        for (size_t i = 0; i < S; i++)
            if (!data[i]->disabled()) rv = false;
        return rv;
    }
    bool disable() {
        bool rv = true;
        for (size_t i = 0; i < S; i++)
            if (!data[i]->disable()) rv = false;
        return rv;
    }
    void enable() {
        for (size_t i = 0; i < S; i++) data[i]->enable();
    }
    bool disable_if_unmodified() {
        bool rv = true;
        for (size_t i = 0; i < S; i++)
            if (!data[i]->disable_if_unmodified()) rv = false;
        return rv;
    }
    bool disable_if_zero() {
        bool rv = true;
        for (size_t i = 0; i < S; i++)
            if (!data[i]->disable_if_zero()) rv = false;
        return rv;
    }
    bool disable_if_reset_value() {
        bool rv = true;
        for (size_t i = 0; i < S; i++)
            if (!data[i]->disable_if_reset_value()) rv = false;
        return rv;
    }
};

#endif /* BF_ASM_ALIAS_ARRAY_H_ */
