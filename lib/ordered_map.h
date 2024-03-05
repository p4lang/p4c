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

#ifndef LIB_ORDERED_MAP_H_
#define LIB_ORDERED_MAP_H_

#include <cassert>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <utility>

// Map is ordered by order of element insertion.
template <class K, class V, class COMP = std::less<K>,
          class ALLOC = std::allocator<std::pair<const K, V>>>
class ordered_map {
 public:
    typedef K key_type;
    typedef V mapped_type;
    typedef std::pair<const K, V> value_type;
    typedef COMP key_compare;
    typedef ALLOC allocator_type;
    typedef value_type &reference;
    typedef const value_type &const_reference;

 private:
    typedef std::list<value_type, ALLOC> list_type;
    typedef typename list_type::iterator list_iterator;
    list_type data;

 public:
    typedef typename list_type::iterator iterator;
    typedef typename list_type::const_iterator const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    class value_compare {
        friend class ordered_map;

     protected:
        COMP comp;
        explicit value_compare(COMP c) : comp(c) {}

     public:
        bool operator()(const value_type &a, const value_type &b) const {
            return comp(a.first, b.first);
        }
    };

 private:
    struct mapcmp {
        COMP comp;
        bool operator()(const K *a, const K *b) const { return comp(*a, *b); }
    };
    using map_alloc = typename std::allocator_traits<ALLOC>::template rebind_alloc<
        std::pair<const K *const, list_iterator>>;
    using map_type = std::map<const K *, list_iterator, mapcmp, map_alloc>;
    map_type data_map;
    void init_data_map() {
        data_map.clear();
        for (auto it = data.begin(); it != data.end(); it++) data_map.emplace(&it->first, it);
    }
    iterator tr_iter(typename map_type::iterator i) {
        if (i == data_map.end()) return data.end();
        return i->second;
    }
    const_iterator tr_iter(typename map_type::const_iterator i) const {
        if (i == data_map.end()) return data.end();
        return i->second;
    }

 public:
    typedef typename map_type::size_type size_type;

    ordered_map() {}
    ordered_map(const ordered_map &a) : data(a.data) { init_data_map(); }
    template <typename InputIt>
    ordered_map(InputIt first, InputIt last) : data(first, last) {
        init_data_map();
    }
    ordered_map(ordered_map &&a) = default; /* move is ok? */
    ordered_map &operator=(const ordered_map &a) {
        /* std::list assignment broken by spec if elements are const... */
        if (this != &a) {
            data.clear();
            for (auto &el : a.data) data.push_back(el);
            init_data_map();
        }
        return *this;
    }
    ordered_map &operator=(ordered_map &&a) = default; /* move is ok? */
    ordered_map(std::initializer_list<value_type> il) : data(il) { init_data_map(); }
    // FIXME add allocator and comparator ctors...

    iterator begin() noexcept { return data.begin(); }
    const_iterator begin() const noexcept { return data.begin(); }
    iterator end() noexcept { return data.end(); }
    const_iterator end() const noexcept { return data.end(); }
    reverse_iterator rbegin() noexcept { return data.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return data.rbegin(); }
    reverse_iterator rend() noexcept { return data.rend(); }
    const_reverse_iterator rend() const noexcept { return data.rend(); }
    const_iterator cbegin() const noexcept { return data.cbegin(); }
    const_iterator cend() const noexcept { return data.cend(); }
    const_reverse_iterator crbegin() const noexcept { return data.crbegin(); }
    const_reverse_iterator crend() const noexcept { return data.crend(); }

    bool empty() const noexcept { return data.empty(); }
    size_type size() const noexcept { return data_map.size(); }
    size_type max_size() const noexcept { return data_map.max_size(); }
    bool operator==(const ordered_map &a) const { return data == a.data; }
    bool operator!=(const ordered_map &a) const { return data != a.data; }
    void clear() {
        data.clear();
        data_map.clear();
    }

    iterator find(const key_type &a) { return tr_iter(data_map.find(&a)); }
    const_iterator find(const key_type &a) const { return tr_iter(data_map.find(&a)); }
    size_type count(const key_type &a) const { return data_map.count(&a); }
    iterator lower_bound(const key_type &a) { return tr_iter(data_map.lower_bound(&a)); }
    const_iterator lower_bound(const key_type &a) const {
        return tr_iter(data_map.lower_bound(&a));
    }
    iterator upper_bound(const key_type &a) { return tr_iter(data_map.upper_bound(&a)); }
    const_iterator upper_bound(const key_type &a) const {
        return tr_iter(data_map.upper_bound(&a));
    }
    iterator upper_bound_pred(const key_type &a) {
        auto ub = data_map.upper_bound(&a);
        if (ub == data_map.begin()) return end();
        return tr_iter(--ub);
    }
    const_iterator upper_bound_pred(const key_type &a) const {
        auto ub = data_map.upper_bound(&a);
        if (ub == data_map.begin()) return end();
        return tr_iter(--ub);
    }

    V &operator[](const K &x) {
        auto it = find(x);
        if (it == data.end()) {
            it = data.emplace(data.end(), x, V());
            data_map.emplace(&it->first, it);
        }
        return it->second;
    }
    V &operator[](K &&x) {
        auto it = find(x);
        if (it == data.end()) {
            it = data.emplace(data.end(), std::move(x), V());
            data_map.emplace(&it->first, it);
        }
        return it->second;
    }
    V &at(const K &x) { return data_map.at(&x)->second; }
    const V &at(const K &x) const { return data_map.at(&x)->second; }

    template <typename KK, typename... VV>
    std::pair<iterator, bool> emplace(KK &&k, VV &&...v) {
        auto it = find(k);
        if (it == data.end()) {
            it = data.emplace(data.end(), std::piecewise_construct_t(), std::forward_as_tuple(k),
                              std::forward_as_tuple(std::forward<VV>(v)...));
            data_map.emplace(&it->first, it);
            return std::make_pair(it, true);
        }
        return std::make_pair(it, false);
    }
    template <typename KK, typename... VV>
    std::pair<iterator, bool> emplace_hint(iterator pos, KK &&k, VV &&...v) {
        /* should be const_iterator pos, but glibc++ std::list is broken */
        auto it = find(k);
        if (it == data.end()) {
            it = data.emplace(pos, std::piecewise_construct_t(), std::forward_as_tuple(k),
                              std::forward_as_tuple(std::forward<VV>(v)...));
            data_map.emplace(&it->first, it);
            return std::make_pair(it, true);
        }
        return std::make_pair(it, false);
    }

    std::pair<iterator, bool> insert(const value_type &v) {
        auto it = find(v.first);
        if (it == data.end()) {
            it = data.insert(data.end(), v);
            data_map.emplace(&it->first, it);
            return std::make_pair(it, true);
        }
        return std::make_pair(it, false);
    }

    /* TODO: should not exist, does not make sense for map that preserves
     * insertion order. This function does not preserve it. */
    std::pair<iterator, bool> insert(iterator pos, const value_type &v) {
        /* should be const_iterator pos, but glibc++ std::list is broken */
        auto it = find(v.first);
        if (it == data.end()) {
            it = data.insert(pos, v);
            data_map.emplace(&it->first, it);
            return std::make_pair(it, true);
        }
        return std::make_pair(it, false);
    }
    template <class InputIterator>
    void insert(InputIterator b, InputIterator e) {
        while (b != e) insert(*b++);
    }

    /* TODO: should not exist, does not make sense for map that preserves
     * insertion order. This function does not preserve it. */
    template <class InputIterator>
    void insert(iterator pos, InputIterator b, InputIterator e) {
        /* should be const_iterator pos, but glibc++ std::list is broken */
        while (b != e) insert(pos, *b++);
    }

    iterator erase(const_iterator pos) {
        auto it = data_map.find(&pos->first);
        assert(it != data_map.end());
        // get the non-const std::list iterator -- libstdc++ is missing
        // std::list::erase(const_iterator) overload
        auto list_it = it->second;
        data_map.erase(it);
        return data.erase(list_it);
    }
    size_type erase(const K &k) {
        auto it = find(k);
        if (it != data.end()) {
            data_map.erase(&k);
            data.erase(it);
            return 1;
        }
        return 0;
    }

    template <class Compare>
    void sort(Compare comp) {
        data.sort(comp);
    }
};

#endif /* LIB_ORDERED_MAP_H_ */
