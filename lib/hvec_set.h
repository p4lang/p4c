/*
Copyright 2023-present Intel

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

#ifndef LIB_HVEC_SET_H_
#define LIB_HVEC_SET_H_

#include <initializer_list>
#include <tuple>
#include <vector>

#include "exceptions.h"
#include "hashvec.h"

template <class KEY, class HASH = std::hash<KEY>, class PRED = std::equal_to<KEY>,
          class ALLOC = std::allocator<KEY>>
class hvec_set : hash_vector_base {
    HASH hf;  // FIXME -- use empty base optimization for these?
    PRED eql;

 public:
    typedef const KEY value_type;
    typedef HASH hasher;
    typedef PRED key_equal;
    typedef ALLOC allocator_type;
    typedef value_type *pointer, *const_pointer, &reference, &const_reference;
    typedef hash_vector_base::lookup_cache lookup_cache;

    explicit hvec_set(size_t icap = 0, const hasher &hf = hasher(),
                      const key_equal &eql = key_equal(),
                      const allocator_type &a = allocator_type())
        : hash_vector_base(false, false, icap), hf(hf), eql(eql), data(a) {
        data.reserve(icap);
    }
    hvec_set(const hvec_set &) = default;
    hvec_set(hvec_set &&) = default;
    hvec_set &operator=(const hvec_set &that) {
        if (this != std::addressof(that)) {
            clear();
            hf = that.hf;
            eql = that.eql;

            data.reserve(that.size());
            insert(that.begin(), that.end());
        }

        return *this;
    }
    hvec_set &operator=(hvec_set &&) = default;
    ~hvec_set() = default;
    template <class ITER>
    hvec_set(ITER begin, ITER end, const hasher &hf = hasher(), const key_equal &eql = key_equal(),
             const allocator_type &a = allocator_type())
        : hash_vector_base(false, false, end - begin), hf(hf), eql(eql), data(a) {
        data.reserve(end - begin);
        for (auto it = begin; it != end; ++it) insert(*it);
    }
    hvec_set(std::initializer_list<value_type> il, const hasher &hf = hasher(),
             const key_equal &eql = key_equal(), const allocator_type &a = allocator_type())
        : hash_vector_base(false, false, il.size()), hf(hf), eql(eql), data(a) {
        data.reserve(il.size());
        for (auto &i : il) insert(i);
    }

 private:
    template <class HVM, class VT>
    class _iter {
        HVM *self;
        size_t idx;

        friend class hvec_set;
        _iter(HVM &s, size_t i) : self(&s), idx(i) {}

     public:
        using value_type = VT;
        using difference_type = ssize_t;
        using pointer = typename HVM::pointer;
        using reference = typename HVM::reference;
        using iterator_category = std::bidirectional_iterator_tag;
        _iter(const _iter &) = default;
        _iter &operator=(const _iter &a) {
            self = a.self;
            idx = a.idx;
            return *this;
        }
        value_type &operator*() const { return self->data[idx]; }
        value_type *operator->() const { return &self->data[idx]; }
        _iter &operator++() {
            do {
                ++idx;
            } while (self->erased[idx]);
            return *this;
        }
        _iter &operator--() {
            do {
                --idx;
            } while (self->erased[idx]);
            return *this;
        }
        _iter operator++(int) {
            auto copy = *this;
            ++*this;
            return copy;
        }
        _iter operator--(int) {
            auto copy = *this;
            --*this;
            return copy;
        }
        bool operator==(const _iter &a) const { return self == a.self && idx == a.idx; }
        bool operator!=(const _iter &a) const { return self != a.self || idx != a.idx; }
        operator _iter<const HVM, const VT>() const {
            return _iter<const HVM, const VT>(*self, idx);
        }
    };

 public:
    typedef _iter<hvec_set, value_type> iterator;
    typedef _iter<const hvec_set, const value_type> const_iterator;
    iterator begin() { return iterator(*this, erased.ffz()); }
    iterator end() { return iterator(*this, data.size()); }
    const_iterator begin() const { return const_iterator(*this, erased.ffz()); }
    const_iterator end() const { return const_iterator(*this, data.size()); }
    const_iterator cbegin() const { return const_iterator(*this, erased.ffz()); }
    const_iterator cend() const { return const_iterator(*this, data.size()); }
    value_type &front() const { return *begin(); }
    value_type &back() const {
        auto it = end();
        return *--it;
    }

    bool empty() const { return inuse == 0; }
    size_t size() const { return inuse; }
    size_t max_size() const { return UINT32_MAX; }
    bool operator==(const hvec_set &a) const {
        if (inuse != a.inuse) return false;
        auto it = begin();
        for (auto &el : a)
            if (el != *it++) return false;
        return true;
    }
    bool operator!=(const hvec_set &a) const { return !(*this == a); }
    void clear() {
        hash_vector_base::clear();
        data.clear();
    }

    iterator find(const KEY &k) {
        hash_vector_base::lookup_cache cache;
        size_t idx = hash_vector_base::find(&k, &cache);
        return idx ? iterator(*this, idx - 1) : end();
    }
    const_iterator find(const KEY &k) const {
        hash_vector_base::lookup_cache cache;
        size_t idx = hash_vector_base::find(&k, &cache);
        return idx ? const_iterator(*this, idx - 1) : end();
    }
    size_t count(const KEY &k) const {
        hash_vector_base::lookup_cache cache;
        size_t idx = hash_vector_base::find(&k, &cache);
        return idx > 0;
    }

    template <typename... KK>
    std::pair<iterator, bool> emplace(KK &&...k) {
        return insert(value_type(std::forward<KK>(k)...));
    }
    std::pair<iterator, bool> insert(const value_type &v) {
        bool new_key = false;
        size_t idx = hv_insert(&v);
        if (idx >= data.size()) {
            idx = data.size();
            data.push_back(v);
            new_key = true;
        } else if ((new_key = erased[idx])) {
            erased[idx] = 0;
            data[idx] = v;
        }
        return std::make_pair(iterator(*this, idx), new_key);
    }
    std::pair<iterator, bool> insert(value_type &&v) {
        bool new_key = false;
        size_t idx = hv_insert(&v);
        if (idx >= data.size()) {
            idx = data.size();
            data.push_back(v);
            new_key = true;
        } else if ((new_key = erased[idx])) {
            erased[idx] = 0;
            data[idx] = std::move(v);
        }
        return std::make_pair(iterator(*this, idx), new_key);
    }

    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last) {
        for (; first != last; ++first) insert(*first);
    }
    void insert(std::initializer_list<value_type> vl) { return insert(vl.begin(), vl.end()); }
    template <class HVM, class VT>
    _iter<HVM, VT> erase(_iter<HVM, VT> it) {
        BUG_CHECK(this == it.self, "incorrect iterator for hvec_set::erase");
        erased[it.idx] = 1;
        // FIXME -- would be better to call dtor here, but that will cause
        // problems with the vector when it is resized or destroyed.  Could
        // use raw memory and manual construct instead.
        data[it.idx] = KEY();
        ++it;
        --inuse;
        return it;
    }
    size_t erase(const KEY &k) {
        size_t idx = remove(&k);
        if (idx + 1 == 0) return 0;
        if (idx < data.size()) {
            data[idx] = KEY();
        }
        return 1;
    }
#ifdef DEBUG
    using hash_vector_base::dump;
#endif

 private:
    std::vector<KEY, ALLOC> data;
    size_t hashfn(const void *a) const override { return hf(*static_cast<const KEY *>(a)); }
    bool cmpfn(const void *a, const void *b) const override {
        return eql(*static_cast<const KEY *>(a), *static_cast<const KEY *>(b));
    }
    bool cmpfn(const void *a, size_t b) const override {
        return eql(*static_cast<const KEY *>(a), data[b]);
    }
    const void *getkey(uint32_t i) const override { return &data[i]; }
    void *getval(uint32_t) override {
        BUG("getval in set");
        return nullptr;
    }
    uint32_t limit() override { return data.size(); }
    void resizedata(size_t sz) override { data.resize(sz); }
    void moveentry(size_t to, size_t from) override { data[to] = data[from]; }
};

#endif /* LIB_HVEC_SET_H_ */
