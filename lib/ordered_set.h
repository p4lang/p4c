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

#ifndef LIB_ORDERED_SET_H_
#define LIB_ORDERED_SET_H_

#include <cassert>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <utility>

// Remembers items in insertion order
template <class T, class COMP = std::less<T>, class ALLOC = std::allocator<T>>
class ordered_set {
 public:
    typedef T key_type;
    typedef T value_type;
    typedef COMP key_compare;
    typedef COMP value_compare;
    typedef ALLOC allocator_type;
    typedef const T &reference;
    typedef const T &const_reference;

 private:
    typedef std::list<T, ALLOC> list_type;
    typedef typename list_type::iterator list_iterator;
    list_type data;

 public:
    // This is a set, so we can't provide any iterator that would make it
    // possible to modify the values in the set and therefore possibly make the
    // set contain duplicate values. Therefore we base all our iterators on
    // list's const_iterator.
    typedef typename list_type::const_iterator iterator;
    typedef typename list_type::const_iterator const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

 private:
    struct mapcmp : std::binary_function<const T *, const T *, bool> {
        COMP comp;
        bool operator()(const T *a, const T *b) const { return comp(*a, *b); }
    };
    using map_alloc =
        typename ALLOC::template rebind<std::pair<const T *const, list_iterator>>::other;
    using map_type = std::map<const T *, list_iterator, mapcmp, map_alloc>;
    map_type data_map;
    void init_data_map() {
        data_map.clear();
        for (auto it = data.begin(); it != data.end(); it++) data_map.emplace(&*it, it);
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
    class sorted_iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
        friend class ordered_set;
        typename map_type::const_iterator iter;
        sorted_iterator(typename map_type::const_iterator it)  // NOLINT(runtime/explicit)
            : iter(it) {}

     public:
        const T &operator*() const { return *iter->first; }
        const T *operator->() const { return iter->first; }
        sorted_iterator operator++() {
            ++iter;
            return *this;
        }
        sorted_iterator operator--() {
            --iter;
            return *this;
        }
        sorted_iterator operator++(int) {
            auto copy = *this;
            ++iter;
            return copy;
        }
        sorted_iterator operator--(int) {
            auto copy = *this;
            --iter;
            return copy;
        }
        bool operator==(const sorted_iterator i) const { return iter == i.iter; }
        bool operator!=(const sorted_iterator i) const { return iter != i.iter; }
    };

    ordered_set() {}
    ordered_set(const ordered_set &a) : data(a.data) { init_data_map(); }
    ordered_set(std::initializer_list<T> init) : data(init) { init_data_map(); }
    template <typename InputIt>
    ordered_set(InputIt first, InputIt last) : data(first, last) {
        init_data_map();
    }
    ordered_set(ordered_set &&a) = default; /* move is ok? */
    ordered_set &operator=(const ordered_set &a) {
        data = a.data;
        init_data_map();
        return *this;
    }
    ordered_set &operator=(ordered_set &&a) = default; /* move is ok? */
    bool operator==(const ordered_set &a) const { return data == a.data; }
    bool operator!=(const ordered_set &a) const { return data != a.data; }
    bool operator<(const ordered_set &a) const {
        // we define this to work INDEPENDENT of the order -- so it is possible to have
        // two ordered_sets where !(a < b) && !(b < a) && !(a == b) -- such sets have the
        // same elements but in a different order.  This is generally what you want if you
        // have a set of ordered_sets (or use ordered_set as a map key).
        auto it = a.data_map.begin();
        for (auto &el : data_map) {
            if (it == a.data_map.end()) return false;
            if (mapcmp()(el.first, it->first)) return true;
            if (mapcmp()(it->first, el.first)) return false;
            ++it;
        }
        return it != a.data_map.end();
    }

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
    sorted_iterator sorted_begin() const noexcept { return data_map.begin(); }
    sorted_iterator sorted_end() const noexcept { return data_map.end(); }

    reference front() const noexcept { return *data.begin(); }
    reference back() const noexcept { return *data.rbegin(); }

    bool empty() const noexcept { return data.empty(); }
    size_type size() const noexcept { return data_map.size(); }
    size_type max_size() const noexcept { return data_map.max_size(); }
    void clear() {
        data.clear();
        data_map.clear();
    }

    iterator find(const T &a) { return tr_iter(data_map.find(&a)); }
    const_iterator find(const T &a) const { return tr_iter(data_map.find(&a)); }
    size_type count(const T &a) const { return data_map.count(&a); }
    iterator upper_bound(const T &a) { return tr_iter(data_map.upper_bound(&a)); }
    const_iterator upper_bound(const T &a) const { return tr_iter(data_map.upper_bound(&a)); }
    iterator lower_bound(const T &a) { return tr_iter(data_map.lower_bound(&a)); }
    const_iterator lower_bound(const T &a) const { return tr_iter(data_map.lower_bound(&a)); }

    std::pair<iterator, bool> insert(const T &v) {
        iterator it = find(v);
        if (it == data.end()) {
            list_iterator it = data.insert(data.end(), v);
            data_map.emplace(&*it, it);
            return std::make_pair(it, true);
        }
        return std::make_pair(it, false);
    }
    std::pair<iterator, bool> insert(T &&v) {
        iterator it = find(v);
        if (it == data.end()) {
            list_iterator it = data.insert(data.end(), std::move(v));
            data_map.emplace(&*it, it);
            return std::make_pair(it, true);
        }
        return std::make_pair(it, false);
    }
    void insert(ordered_set::const_iterator begin, ordered_set::const_iterator end) {
        for (auto it = begin; it != end; ++it) insert(*it);
    }
    iterator insert(const_iterator pos, const T &v) {
        iterator it = find(v);
        if (it == data.end()) {
            list_iterator it = data.insert(pos, v);
            data_map.emplace(&*it, it);
            return it;
        }
        return it;
    }
    iterator insert(const_iterator pos, T &&v) {
        iterator it = find(v);
        if (it == data.end()) {
            list_iterator it = data.insert(pos, std::move(v));
            data_map.emplace(&*it, it);
            return it;
        }
        return it;
    }

    void push_back(const T &v) {
        iterator it = find(v);
        if (it == data.end()) {
            list_iterator it = data.insert(data.end(), v);
            data_map.emplace(&*it, it);
        } else {
            data.splice(data.end(), data, it);
        }
    }
    void push_back(T &&v) {
        iterator it = find(v);
        if (it == data.end()) {
            list_iterator it = data.insert(data.end(), std::move(v));
            data_map.emplace(&*it, it);
        } else {
            data.splice(data.end(), data, it);
        }
    }

    template <class... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        auto it = data.emplace(data.end(), std::forward<Args>(args)...);
        auto old = find(*it);
        if (old == data.end()) {
            data_map.emplace(&*it, it);
            return std::make_pair(it, true);
        } else {
            data.erase(it);
            return std::make_pair(old, false);
        }
    }

    template <class... Args>
    std::pair<iterator, bool> emplace_back(Args &&...args) {
        auto it = data.emplace(data.end(), std::forward<Args>(args)...);
        auto old = find(*it);
        if (old != data.end()) {
            data.erase(old);
        }
        data_map.emplace(&*it, it);
        return std::make_pair(it, true);
    }

    iterator erase(const_iterator pos) {
        auto it = data_map.find(&*pos);
        assert(it != data_map.end());
        // get the non-const std::list iterator -- libstdc++ is missing
        // std::list::erase(const_iterator) overload
        auto list_it = it->second;
        data_map.erase(it);
        return data.erase(list_it);
    }
    size_type erase(const T &v) {
        auto it = find(v);
        if (it != data.end()) {
            data_map.erase(&v);
            data.erase(it);
            return 1;
        }
        return 0;
    }
};

template <class T, class C1, class A1, class U>
inline auto operator|=(ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), a) {
    for (auto &el : b) a.insert(el);
    return a;
}
template <class T, class C1, class A1, class U>
inline auto operator-=(ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), a) {
    for (auto &el : b) a.erase(el);
    return a;
}
template <class T, class C1, class A1, class U>
inline auto operator&=(ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), a) {
    for (auto it = a.begin(); it != a.end();) {
        if (b.count(*it))
            ++it;
        else
            it = a.erase(it);
    }
    return a;
}

template <class T, class C1, class A1, class U>
inline auto contains(const ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), true) {
    for (auto &el : b)
        if (!a.count(el)) return false;
    return true;
}
template <class T, class C1, class A1, class U>
inline auto intersects(const ordered_set<T, C1, A1> &a, const U &b) -> decltype(b.begin(), true) {
    for (auto &el : b)
        if (a.count(el)) return true;
    return false;
}

#endif /* LIB_ORDERED_SET_H_ */
