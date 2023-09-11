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

#ifndef LIB_HVEC_MAP_H_
#define LIB_HVEC_MAP_H_

#include <tuple>
#include <vector>

#include "exceptions.h"
#include "hashvec.h"

template <class KEY, class VAL, class HASH = std::hash<KEY>, class PRED = std::equal_to<KEY>,
          class ALLOC = std::allocator<std::pair<const KEY, VAL>>>
class hvec_map : hash_vector_base {
    HASH hf;  // FIXME -- use empty base optimization for these?
    PRED eql;

 public:
    typedef KEY key_type;
    typedef std::pair<const KEY, VAL> value_type;
    typedef VAL mapped_type;
    typedef HASH hasher;
    typedef PRED key_equal;
    typedef ALLOC allocator_type;

    typedef typename std::vector<value_type>::pointer pointer;
    typedef typename std::vector<value_type>::const_pointer const_pointer;
    typedef typename std::vector<value_type>::reference reference;
    typedef typename std::vector<value_type>::const_reference const_reference;

    typedef hash_vector_base::lookup_cache lookup_cache;

    explicit hvec_map(size_t icap = 0, const hasher &hf = hasher(),
                      const key_equal &eql = key_equal(),
                      const allocator_type &a = allocator_type())
        : hash_vector_base(true, false, icap), hf(hf), eql(eql), data(a) {
        data.reserve(icap);
    }
    hvec_map(const hvec_map &) = default;
    hvec_map(hvec_map &&) = default;
    hvec_map &operator=(const hvec_map &) = default;
    hvec_map &operator=(hvec_map &&) = default;
    ~hvec_map() = default;
    template <class ITER>
    hvec_map(ITER begin, ITER end, const hasher &hf = hasher(), const key_equal &eql = key_equal(),
             const allocator_type &a = allocator_type())
        : hash_vector_base(true, false, end - begin), hf(hf), eql(eql), data(a) {
        data.reserve(end - begin);
        for (auto it = begin; it != end; ++it) insert(*it);
    }
    hvec_map(std::initializer_list<value_type> il, const hasher &hf = hasher(),
             const key_equal &eql = key_equal(), const allocator_type &a = allocator_type())
        : hash_vector_base(true, false, il.size()), hf(hf), eql(eql), data(a) {
        data.reserve(il.size());
        for (auto &i : il) insert(i);
    }

 private:
    template <class HVM, class VT>
    class _iter {
        HVM *self;
        size_t idx;

        friend class hvec_map;
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
    typedef _iter<hvec_map, value_type> iterator;
    typedef _iter<const hvec_map, const value_type> const_iterator;
    iterator begin() { return iterator(*this, erased.ffz()); }
    iterator end() { return iterator(*this, data.size()); }
    const_iterator begin() const { return const_iterator(*this, erased.ffz()); }
    const_iterator end() const { return const_iterator(*this, data.size()); }
    const_iterator cbegin() const { return const_iterator(*this, erased.ffz()); }
    const_iterator cend() const { return const_iterator(*this, data.size()); }

    bool empty() const { return inuse == 0; }
    size_t size() const { return inuse; }
    size_t max_size() const { return UINT32_MAX; }
    bool operator==(const hvec_map &a) {
        if (inuse != a.inuse) return false;
        auto it = begin();
        for (auto &el : a)
            if (el != *it++) return false;
        return true;
    }
    bool operator!=(const hvec_map &a) { return !(*this == a); }
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

    // FIXME -- how to do this without duplicating the code for lvalue/rvalue?
    VAL &operator[](const KEY &k) {
        size_t idx = hv_insert(&k);
        if (idx >= data.size()) {
            data.emplace_back(k, VAL());
            return data.back().second;
        } else {
            if (erased[idx]) {
                erased[idx] = 0;
                const_cast<KEY &>(data[idx].first) = k;
                data[idx].second = VAL();
            }
            return data[idx].second;
        }
    }
    VAL &operator[](KEY &&k) {
        size_t idx = hv_insert(&k);
        if (idx >= data.size()) {
            data.emplace_back(std::move(k), VAL());
            return data.back().second;
        } else {
            if (erased[idx]) {
                erased[idx] = 0;
                const_cast<KEY &>(data[idx].first) = std::move(k);
                data[idx].second = VAL();
            }
            return data[idx].second;
        }
    }

    // FIXME -- how to do this without duplicating the code for lvalue/rvalue?
    template <typename... VV>
    std::pair<iterator, bool> emplace(const KEY &k, VV &&...v) {
        bool new_key = false;
        size_t idx = hv_insert(&k);
        if (idx >= data.size()) {
            idx = data.size();
            data.emplace_back(std::piecewise_construct_t(), std::forward_as_tuple(k),
                              std::forward_as_tuple(std::forward<VV>(v)...));
            new_key = true;
        } else {
            if ((new_key = erased[idx])) {
                erased[idx] = 0;
                const_cast<KEY &>(data[idx].first) = k;
                data[idx].second = VAL(std::forward<VV>(v)...);
            } else {
                data[idx].second = VAL(std::forward<VV>(v)...);
            }
        }
        return std::make_pair(iterator(*this, idx), new_key);
    }
    template <typename... VV>
    std::pair<iterator, bool> emplace(KEY &&k, VV &&...v) {
        bool new_key = false;
        size_t idx = hv_insert(&k);
        if (idx >= data.size()) {
            idx = data.size();
            data.emplace_back(std::piecewise_construct_t(), std::forward_as_tuple(std::move(k)),
                              std::forward_as_tuple(std::forward<VV>(v)...));
            new_key = true;
        } else {
            if ((new_key = erased[idx])) {
                erased[idx] = 0;
                const_cast<KEY &>(data[idx].first) = std::move(k);
                data[idx].second = VAL(std::forward<VV>(v)...);
            } else {
                data[idx].second = VAL(std::forward<VV>(v)...);
            }
        }
        return std::make_pair(iterator(*this, idx), new_key);
    }
    std::pair<iterator, bool> insert(const value_type &v) {
        bool new_key = false;
        size_t idx = hv_insert(&v.first);
        if (idx >= data.size()) {
            idx = data.size();
            data.push_back(v);
            new_key = true;
        } else {
            if ((new_key = erased[idx])) {
                erased[idx] = 0;
                const_cast<KEY &>(data[idx].first) = v.first;
                data[idx].second = v.second;
            } else {
                data[idx].second = v.second;
            }
        }
        return std::make_pair(iterator(*this, idx), new_key);
    }
    template <class HVM, class VT>
    _iter<HVM, VT> erase(_iter<HVM, VT> it) {
        BUG_CHECK(this == it.self, "incorrect iterator for hvec_map::erase");
        erased[it.idx] = 1;
        // FIXME -- would be better to call dtor here, but that will cause
        // problems with the vector when it resized or is destroyed.  Could
        // use raw memory and manual construct instead.
        const_cast<KEY &>(data[it.idx].first) = KEY();
        data[it.idx].second = VAL();
        ++it;
        --inuse;
        return it;
    }
    size_t erase(const KEY &k) {
        size_t idx = remove(&k);
        if (idx + 1 == 0) return 0;
        if (idx < data.size()) {
            const_cast<KEY &>(data[idx].first) = KEY();
            data[idx].second = VAL();
        }
        return 1;
    }
#ifdef DEBUG
    using hash_vector_base::dump;
#endif

 private:
    std::vector<value_type, ALLOC> data;
    size_t hashfn(const void *a) const override { return hf(*static_cast<const KEY *>(a)); }
    bool cmpfn(const void *a, const void *b) const override {
        return eql(*static_cast<const KEY *>(a), *static_cast<const KEY *>(b));
    }
    bool cmpfn(const void *a, size_t b) const override {
        return eql(*static_cast<const KEY *>(a), data[b].first);
    }
    const void *getkey(uint32_t i) const override { return &data[i].first; }
    void *getval(uint32_t i) override { return &data[i].second; }
    uint32_t limit() override { return data.size(); }
    void resizedata(size_t sz) override { data.resize(sz); }
    void moveentry(size_t to, size_t from) override {
        // can't just assign, as the keys are const -- need to destruct & construct
        data[to].~value_type();
        new (&data[to]) value_type(std::move(data[from]));
    }
};

// XXX(seth): We use this namespace to hide our get() overloads from ADL. GCC
// 4.8 has a bug which causes these overloads to be considered when get() is
// called on a type in the global namespace, even if the number of arguments
// doesn't match up, which can trigger template instantiations that cause
// errors.
namespace GetImpl {

template <class K, class T, class V, class Comp, class Alloc>
inline V get(const hvec_map<K, V, Comp, Alloc> &m, T key, V def = V()) {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    return def;
}

template <class K, class T, class V, class Comp, class Alloc>
inline V *getref(hvec_map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) return &it->second;
    return 0;
}

template <class K, class T, class V, class Comp, class Alloc>
inline const V *getref(const hvec_map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) return &it->second;
    return 0;
}

template <class K, class T, class V, class Comp, class Alloc>
inline V get(const hvec_map<K, V, Comp, Alloc> *m, T key, V def = V()) {
    return m ? get(*m, key, def) : def;
}

template <class K, class T, class V, class Comp, class Alloc>
inline V *getref(hvec_map<K, V, Comp, Alloc> *m, T key) {
    return m ? getref(*m, key) : 0;
}

template <class K, class T, class V, class Comp, class Alloc>
inline const V *getref(const hvec_map<K, V, Comp, Alloc> *m, T key) {
    return m ? getref(*m, key) : 0;
}

}  // namespace GetImpl
using namespace GetImpl;  // NOLINT(build/namespaces)

#endif /* LIB_HVEC_MAP_H_ */
