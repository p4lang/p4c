/*
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
#ifndef LIB_FLAT_MAP_H_
#define LIB_FLAT_MAP_H_

#include <algorithm>
#include <functional>
#include <vector>

namespace P4 {

/// A header-only implementation of a memory-efficient flat_map.
/// TODO: Replace this map with std::flat_map once available in C++23:
/// https://en.cppreference.com/w/cpp/container/flat_map
template <typename K, typename V, typename Compare = std::less<>,
          typename Container = std::vector<std::pair<K, V>>>
struct flat_map {
    using key_type = K;
    using mapped_type = V;
    using value_type = typename Container::value_type;
    using key_compare = Compare;

    struct value_compare {
        bool operator()(const value_type &lhs, const value_type &rhs) const {
            return key_compare()(lhs.first, rhs.first);
        }
    };

    using allocator_type = typename Container::allocator_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using pointer = typename Container::pointer;
    using const_pointer = typename Container::const_pointer;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using reverse_iterator = typename Container::reverse_iterator;
    using const_reverse_iterator = typename Container::const_reverse_iterator;
    using difference_type = typename Container::difference_type;
    using size_type = typename Container::size_type;

    flat_map() = default;

    template <typename It>
    flat_map(It begin, It end) {
        insert(begin, end);
    }

    flat_map(std::initializer_list<value_type> il) : flat_map(il.begin(), il.end()) {}

    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    const_iterator cend() const { return data_.cend(); }
    reverse_iterator rbegin() { return data_.rbegin(); }
    reverse_iterator rend() { return data_.rend(); }
    const_reverse_iterator rbegin() const { return data_.rbegin(); }
    const_reverse_iterator rend() const { return data_.rend(); }
    const_reverse_iterator crbegin() const { return data_.crbegin(); }
    const_reverse_iterator crend() const { return data_.crend(); }

    bool empty() const { return data_.empty(); }
    size_type size() const { return data_.size(); }
    size_type max_size() const { return data_.max_size(); }
    size_type capacity() const { return data_.capacity(); }
    void reserve(size_type size) { data_.reserve(size); }
    void shrink_to_fit() { data_.shrink_to_fit(); }
    size_type bytes_used() const { return capacity() * sizeof(value_type) + sizeof(data_); }

    mapped_type &operator[](const key_type &key) {
        KeyOrValueCompare comp;
        auto lower = lower_bound(key);
        if (lower == end() || comp(key, *lower))
            return data_.emplace(lower, key, mapped_type())->second;

        return lower->second;
    }

    mapped_type &operator[](key_type &&key) {
        KeyOrValueCompare comp;
        auto lower = lower_bound(key);
        if (lower == end() || comp(key, *lower))
            return data_.emplace(lower, std::move(key), mapped_type())->second;

        return lower->second;
    }

    template <class Key>
    mapped_type &at(const Key &key) {
        auto found = lower_bound(key);
        if (found == end()) throw std::out_of_range("key is out of range");
        return found->second;
    }
    template <class Key>
    const mapped_type &at(const Key &key) const {
        auto found = lower_bound(key);
        if (found == end()) throw std::out_of_range("key is out of range");
        return found->second;
    }

    std::pair<iterator, bool> insert(value_type &&value) { return emplace(std::move(value)); }

    std::pair<iterator, bool> insert(const value_type &value) { return emplace(value); }

    iterator insert(const_iterator hint, value_type &&value) {
        return emplace_hint(hint, std::move(value));
    }

    iterator insert(const_iterator hint, const value_type &value) {
        return emplace_hint(hint, value);
    }

    template <typename It>
    void insert(It begin, It end) {
        // If we need to increase the capacity, utilize this fact and emplace
        // the stuff.
        for (; begin != end && size() == capacity(); ++begin) {
            emplace(*begin);
        }
        if (begin == end) return;

        // If we don't need to increase capacity, then we can use a more efficient
        // insert method where everything is just put in the same vector
        // and then merge in place.
        size_type size_before = data_.size();
        try {
            for (size_t i = capacity(); i > size_before && begin != end; --i, ++begin) {
                data_.emplace_back(*begin);
            }
        } catch (...) {
            // If emplace_back throws an exception, the easiest way to make sure
            // that our invariants are still in place is to resize to the state
            // we were in before
            for (size_t i = data_.size(); i > size_before; --i) {
                data_.pop_back();
            }
            throw;
        }

        value_compare comp;
        auto mid = data_.begin() + size_before;
        std::stable_sort(mid, data_.end(), comp);
        std::inplace_merge(data_.begin(), mid, data_.end(), comp);
        data_.erase(std::unique(data_.begin(), data_.end(), std::not_fn(comp)), data_.end());

        // Make sure that we inserted at least one element before
        // recursing. Otherwise we'd recurse too often if we were to insert the
        // same element many times
        if (data_.size() == size_before) {
            for (; begin != end; ++begin) {
                if (emplace(*begin).second) {
                    ++begin;
                    break;
                }
            }
        }

        // Insert the remaining elements that didn't fit by calling this function recursively.
        return insert(begin, end);
    }

    void insert(std::initializer_list<value_type> il) { insert(il.begin(), il.end()); }

    iterator erase(iterator it) { return data_.erase(it); }

    iterator erase(const_iterator it) { return erase(iterator_const_cast(it)); }

    size_type erase(const key_type &key) {
        auto found = find(key);
        if (found == end()) return 0;
        erase(found);
        return 1;
    }

    iterator erase(const_iterator first, const_iterator last) {
        return data_.erase(iterator_const_cast(first), iterator_const_cast(last));
    }

    void swap(flat_map &other) { data_.swap(other.data_); }

    void clear() { data_.clear(); }

    template <typename First, typename... Args>
    std::pair<iterator, bool> emplace(First &&first, Args &&...args) {
        KeyOrValueCompare comp;
        auto lower_bound = std::lower_bound(data_.begin(), data_.end(), first, comp);
        if (lower_bound == data_.end() || comp(first, *lower_bound))
            return {
                data_.emplace(lower_bound, std::forward<First>(first), std::forward<Args>(args)...),
                true};

        return {lower_bound, false};
    }

    std::pair<iterator, bool> emplace() { return emplace(value_type()); }

    template <typename First, typename... Args>
    iterator emplace_hint(const_iterator hint, First &&first, Args &&...args) {
        KeyOrValueCompare comp;
        if (hint == cend() || comp(first, *hint)) {
            if (hint == cbegin() || comp(*(hint - 1), first))
                return data_.emplace(iterator_const_cast(hint), std::forward<First>(first),
                                     std::forward<Args>(args)...);

            return emplace(std::forward<First>(first), std::forward<Args>(args)...).first;
        } else if (!comp(*hint, first)) {
            return begin() + (hint - cbegin());
        }

        return emplace(std::forward<First>(first), std::forward<Args>(args)...).first;
    }

    iterator emplace_hint(const_iterator hint) { return emplace_hint(hint, value_type()); }

    key_compare key_comp() const { return key_compare(); }
    value_compare value_comp() const { return value_compare(); }

    template <typename T>
    iterator find(const T &key) {
        return binary_find(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    const_iterator find(const T &key) const {
        return binary_find(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    size_type count(const T &key) const {
        return std::binary_search(begin(), end(), key, KeyOrValueCompare()) ? 1 : 0;
    }
    template <typename T>
    iterator lower_bound(const T &key) {
        return std::lower_bound(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    const_iterator lower_bound(const T &key) const {
        return std::lower_bound(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    iterator upper_bound(const T &key) {
        return std::upper_bound(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    const_iterator upper_bound(const T &key) const {
        return std::upper_bound(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    std::pair<iterator, iterator> equal_range(const T &key) {
        return std::equal_range(begin(), end(), key, KeyOrValueCompare());
    }
    template <typename T>
    std::pair<const_iterator, const_iterator> equal_range(const T &key) const {
        return std::equal_range(begin(), end(), key, KeyOrValueCompare());
    }
    allocator_type get_allocator() const { return data_.get_allocator(); }

    bool operator==(const flat_map &other) const { return data_ == other.data_; }
    bool operator!=(const flat_map &other) const { return !(*this == other); }
    bool operator<(const flat_map &other) const { return data_ < other.data_; }
    bool operator>(const flat_map &other) const { return other < *this; }
    bool operator<=(const flat_map &other) const { return !(other < *this); }
    bool operator>=(const flat_map &other) const { return !(*this < other); }

 private:
    Container data_;

    iterator iterator_const_cast(const_iterator it) { return begin() + (it - cbegin()); }

    struct KeyOrValueCompare {
        template <typename T, typename U>
        bool operator()(const T &lhs, const U &rhs) const {
            return key_compare()(extract(lhs), extract(rhs));
        }

     private:
        const key_type &extract(const value_type &v) const { return v.first; }

        template <typename Key>
        const Key &extract(const Key &k) const {
            return k;
        }
    };

    template <typename It, typename T, typename Comp>
    static It binary_find(It begin, It end, const T &value, const Comp &cmp) {
        auto lower_bound = std::lower_bound(begin, end, value, cmp);
        if (lower_bound == end || cmp(value, *lower_bound)) return end;

        return lower_bound;
    }
};

template <typename K, typename V, typename C, typename Cont>
void swap(flat_map<K, V, C, Cont> &lhs, flat_map<K, V, C, Cont> &rhs) {
    lhs.swap(rhs);
}

}  // namespace P4

#endif  // LIB_FLAT_MAP_H_
