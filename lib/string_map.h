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

#ifndef LIB_STRING_MAP_H_
#define LIB_STRING_MAP_H_

#include <initializer_list>
#include <list>
#include <stdexcept>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "cstring.h"

/// Map with string keys that is ordered by order of element insertion.
/// Use this map only when stable iteration order is significant. For everything
/// else consider using some other unordered containers with cstring keys. Never
/// use std::map<cstring, ...> or ordered_map<cstring, ...> as these maps will
/// have significant overhead due to lexicographical ordering of the keys.
///
/// For this particular implementation:
///  * Key are stored as cstrings in the underlying abseil hash map.
///  * Heterogenous lookup (with std::string_view keys) is supported. Special
///    care is done not to create cstrings in case if they are not in the map
///    already
///  * Values are stored in std::list, similar to ordered_map.
template <class V>
class string_map {
 public:
    using key_type = cstring;
    using mapped_type = V;
    using value_type = std::pair<cstring, V>;
    using reference = value_type &;
    using const_reference = const value_type &;

 private:
    // TODO: Allow different containers (e.g. vector / small vector)
    using list_type = std::list<value_type>;
    using list_iterator = typename list_type::iterator;
    list_type data;

 public:
    using iterator = typename list_type::iterator;
    using const_iterator = typename list_type::const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

 private:
    using map_type = absl::flat_hash_map<cstring, list_iterator>;
    map_type data_map;
    void init_data_map() {
        data_map.clear();
        for (auto it = data.begin(); it != data.end(); it++) data_map.emplace(it->first, it);
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
    using size_type = typename map_type::size_type;

    string_map() = default;
    string_map(const string_map &a) : data(a.data) { init_data_map(); }
    template <typename InputIt>
    string_map(InputIt first, InputIt last) {
        insert(first, last);
    }
    string_map(string_map &&a) = default;
    string_map &operator=(const string_map &a) {
        if (this != &a) {
            data.clear();
            data.insert(data.end(), a.data.begin(), a.data.end());
            init_data_map();
        }
        return *this;
    }
    string_map &operator=(string_map &&a) = default;
    string_map(std::initializer_list<value_type> il) { insert(il.begin(), il.end()); }

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
    /// As the map is ordered, two maps are only considered equal if the elements
    /// are inserted in the same order. Same as with ordered_map though.
    bool operator==(const string_map &a) const { return data == a.data; }
    bool operator!=(const string_map &a) const { return data != a.data; }
    void clear() {
        data.clear();
        data_map.clear();
    }

    iterator find(cstring a) { return tr_iter(data_map.find(a)); }
    /// Functions below do have `std::string_view` versions. Here we are having
    /// important special case: if `a` is not something that was interned, we do
    /// not copy / intern it, we know for sure that `a` is not in the map and we
    /// do not need to perform a lookup.
    iterator find(std::string_view a) {
        cstring key = cstring::get_cached(a);
        if (key.isNull()) return data.end();

        return tr_iter(data_map.find(key));
    }
    const_iterator find(cstring a) const { return tr_iter(data_map.find(a)); }
    const_iterator find(std::string_view a) const {
        cstring key = cstring::get_cached(a);
        if (key.isNull()) return data.end();

        return tr_iter(data_map.find(key));
    }

    size_type count(cstring a) const { return data_map.count(a); }
    size_type count(std::string_view a) const {
        cstring key = cstring::get_cached(a);
        if (key.isNull()) return 0;

        return data_map.count(key);
    }

    bool contains(cstring a) const { return data_map.contains(a); }
    bool contains(std::string_view a) const {
        cstring key = cstring::get_cached(a);
        if (key.isNull()) return false;

        return data_map.contains(key);
    }

    template <typename Key>
    V &operator[](Key &&k) {
        auto it = find(key_type(std::forward<Key>(k)));
        if (it == data.end()) {
            it = data.emplace(data.end(), std::piecewise_construct, std::forward_as_tuple(k),
                              std::forward_as_tuple());
            data_map.emplace(it->first, it);
        }
        return it->second;
    }

    template <typename Key>
    V &at(Key &&k) {
        auto it = find(std::forward<Key>(k));
        if (it == data.end()) throw std::out_of_range("string_map::at");
        return it->second;
    }
    template <typename Key>
    const V &at(Key &&k) const {
        auto it = find(std::forward<Key>(k));
        if (it == data.end()) throw std::out_of_range("string_map::at");
        return it->second;
    }

    template <typename Key, typename... VV>
    std::pair<iterator, bool> emplace(Key &&k, VV &&...v) {
        auto it = find(key_type(std::forward<Key>(k)));
        if (it == data.end()) {
            it = data.emplace(data.end(), std::piecewise_construct, std::forward_as_tuple(k),
                              std::forward_as_tuple(std::forward<VV>(v)...));
            data_map.emplace(it->first, it);
            return {it, true};
        }
        return {it, false};
    }

    std::pair<iterator, bool> insert(value_type &&value) {
        return emplace(std::move(value.first), std::move(value.second));
    }
    std::pair<iterator, bool> insert(const value_type &value) {
        return emplace(value.first, value.second);
    }

    template <class InputIterator>
    void insert(InputIterator b, InputIterator e) {
        while (b != e) insert(*b++);
    }

    iterator erase(const_iterator pos) {
        auto it = data_map.find(pos->first);
        assert(it != data_map.end());
        // get the non-const std::list iterator -- libstdc++ is missing
        // std::list::erase(const_iterator) overload
        auto list_it = it->second;
        data_map.erase(it);
        return data.erase(list_it);
    }
    size_type erase(cstring k) {
        auto it = find(k);
        if (it != data.end()) {
            data_map.erase(k);
            data.erase(it);
            return 1;
        }
        return 0;
    }
    size_type erase(std::string_view k) {
        auto it = find(k);
        if (it != data.end()) {
            data_map.erase(it->first);
            data.erase(it);
            return 1;
        }
        return 0;
    }

    void swap(string_map &other) {
        using std::swap;
        swap(data, other.data);
        swap(data_map, other.data_map);
    }
};

#endif /* LIB_STRING_MAP_H_ */
