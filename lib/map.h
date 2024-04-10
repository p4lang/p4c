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

#ifndef LIB_MAP_H_
#define LIB_MAP_H_

#include <iterator>
#include <optional>

/// Given a map and a key, return the value corresponding to the key in the map,
/// or a given default value if the key doesn't exist in the map.
template <typename Map, typename Key>
typename Map::mapped_type get(const Map &m, const Key &key) {
    auto it = m.find(key);
    return it != m.end() ? it->second : typename Map::mapped_type{};
}
template <class Map, typename Key, typename Value>
typename Map::mapped_type get(const Map &m, const Key &key, Value &&def) {
    using M = typename Map::mapped_type;
    auto it = m.find(key);
    return it != m.end() ? it->second : static_cast<M>(std::forward<Value>(def));
}

/// Given a map and a key, return the pointer to value corresponding to the key
/// in the map, or a nullptr if the key doesn't exist in the map.
template <typename Map, typename Key>
const typename Map::mapped_type *getref(const Map &m, const Key &key) {
    auto it = m.find(key);
    return it != m.end() ? &it->second : nullptr;
}

template <typename Map, typename Key>
typename Map::mapped_type *getref(Map &m, const Key &key) {
    auto it = m.find(key);
    return it != m.end() ? &it->second : nullptr;
}

// Given a map and a key, return a optional<V> if the key exists and None if the
// key does not exist in the map.
template <class Map, typename Key>
std::optional<typename Map::mapped_type> get_optional(const Map &m, const Key &key) {
    auto it = m.find(key);
    if (it != m.end()) return std::optional<typename Map::mapped_type>(it->second);

    return {};
}

template <typename Map, typename Key>
typename Map::mapped_type get(const Map *m, const Key &key) {
    return m ? get(*m, key) : typename Map::mapped_type{};
}

template <class Map, typename Key, typename Value>
typename Map::mapped_type get(const Map *m, const Key &key, Value &&def) {
    return m ? get(*m, key, std::forward(def)) : typename Map::mapped_type{};
}

template <typename Map, typename Key>
const typename Map::mapped_type *getref(const Map *m, const Key &key) {
    return m ? getref(*m, key) : nullptr;
}

template <typename Map, typename Key>
typename Map::mapped_type *getref(Map *m, const Key &key) {
    return m ? getref(*m, key) : nullptr;
}

/* iterate over the keys in a map */
template <class PairIter>
class IterKeys {
    class iterator {
        PairIter it;

     public:
        using iterator_category = typename std::iterator_traits<PairIter>::iterator_category;
        using value_type = typename std::iterator_traits<PairIter>::value_type::first_type;
        using difference_type = typename std::iterator_traits<PairIter>::difference_type;
        using pointer = decltype(&it->first);
        using reference = decltype(*&it->first);

        explicit iterator(PairIter i) : it(i) {}
        iterator &operator++() {
            ++it;
            return *this;
        }
        iterator &operator--() {
            --it;
            return *this;
        }
        iterator operator++(int) {
            auto copy = *this;
            ++it;
            return copy;
        }
        iterator operator--(int) {
            auto copy = *this;
            --it;
            return copy;
        }
        bool operator==(const iterator &i) const { return it == i.it; }
        bool operator!=(const iterator &i) const { return it != i.it; }
        reference operator*() const { return it->first; }
        pointer operator->() const { return &it->first; }
    } b, e;

 public:
    template <class U>
    explicit IterKeys(U &map) : b(map.begin()), e(map.end()) {}
    IterKeys(PairIter b, PairIter e) : b(b), e(e) {}
    iterator begin() const { return b; }
    iterator end() const { return e; }
};

template <class Map>
IterKeys<typename Map::iterator> Keys(Map &m) {
    return IterKeys<typename Map::iterator>(m);
}

template <class Map>
IterKeys<typename Map::const_iterator> Keys(const Map &m) {
    return IterKeys<typename Map::const_iterator>(m);
}

template <class PairIter>
IterKeys<PairIter> Keys(std::pair<PairIter, PairIter> range) {
    return IterKeys<PairIter>(range.first, range.second);
}

/* iterate over the values in a map */
template <class PairIter>
class IterValues {
    class iterator {
        PairIter it;

     public:
        using iterator_category = typename std::iterator_traits<PairIter>::iterator_category;
        using value_type = typename std::iterator_traits<PairIter>::value_type::second_type;
        using difference_type = typename std::iterator_traits<PairIter>::difference_type;
        using pointer = decltype(&it->second);
        using reference = decltype(*&it->second);

        explicit iterator(PairIter i) : it(i) {}
        iterator &operator++() {
            ++it;
            return *this;
        }
        iterator &operator--() {
            --it;
            return *this;
        }
        iterator operator++(int) {
            auto copy = *this;
            ++it;
            return copy;
        }
        iterator operator--(int) {
            auto copy = *this;
            --it;
            return copy;
        }
        bool operator==(const iterator &i) const { return it == i.it; }
        bool operator!=(const iterator &i) const { return it != i.it; }
        reference operator*() const { return it->second; }
        pointer operator->() const { return &it->second; }
    } b, e;

 public:
    using value_type = typename std::iterator_traits<PairIter>::value_type::second_type;

    template <class U>
    explicit IterValues(U &map) : b(map.begin()), e(map.end()) {}
    IterValues(PairIter b, PairIter e) : b(b), e(e) {}
    iterator begin() const { return b; }
    iterator end() const { return e; }
};

template <class Map>
IterValues<typename Map::iterator> Values(Map &m) {
    return IterValues<typename Map::iterator>(m);
}

template <class Map>
IterValues<typename Map::const_iterator> Values(const Map &m) {
    return IterValues<typename Map::const_iterator>(m);
}

template <class PairIter>
IterValues<PairIter> Values(std::pair<PairIter, PairIter> range) {
    return IterValues<PairIter>(range.first, range.second);
}

/* iterate over the values for a single key in a multimap */
template <class M>
class MapForKey {
    M &map;
    typename M::key_type key;
    class iterator {
        using MapIt = decltype(map.begin());

        const MapForKey &self;
        MapIt it;

     public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename M::value_type;
        using difference_type = typename std::iterator_traits<MapIt>::difference_type;
        using pointer = decltype(&it->second);
        using reference = decltype(*&it->second);

        iterator(const MapForKey &s, MapIt i) : self(s), it(std::move(i)) {}
        iterator &operator++() {
            if (++it != self.map.end() && it->first != self.key) it = self.map.end();
            return *this;
        }
        iterator operator++(int) {
            auto copy = *this;
            ++*this;
            return copy;
        }
        bool operator==(const iterator &i) const { return it == i.it; }
        bool operator!=(const iterator &i) const { return it != i.it; }
        reference operator*() const { return it->second; }
        pointer operator->() const { return &it->second; }
    };

 public:
    MapForKey(M &m, typename M::key_type k) : map(m), key(k) {}
    iterator begin() const { return iterator(*this, map.find(key)); }
    iterator end() const { return iterator(*this, map.end()); }
};

template <class M>
MapForKey<M> ValuesForKey(M &m, typename M::key_type k) {
    return MapForKey<M>(m, k);
}

#endif /* LIB_MAP_H_ */
