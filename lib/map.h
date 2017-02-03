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

#ifndef P4C_LIB_MAP_H_
#define P4C_LIB_MAP_H_

#include <map>

// XXX(seth): We use this namespace to hide our get() overloads from ADL. GCC
// 4.8 has a bug which causes these overloads to be considered when get() is
// called on a type in the global namespace, even if the number of arguments
// doesn't match up, which can trigger template instantiations that cause
// errors.
namespace GetImpl {

template<class K, class T, class V, class Comp, class Alloc>
inline V get(const std::map<K, V, Comp, Alloc> &m, T key, V def = V()) {
    auto it = m.find(key);
    if (it != m.end()) return it->second;
    return def; }

template<class K, class T, class V, class Comp, class Alloc>
inline V *getref(std::map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) return &it->second;
    return 0; }

template<class K, class T, class V, class Comp, class Alloc>
inline const V *getref(const std::map<K, V, Comp, Alloc> &m, T key) {
    auto it = m.find(key);
    if (it != m.end()) return &it->second;
    return 0; }

template<class K, class T, class V, class Comp, class Alloc>
inline V get(const std::map<K, V, Comp, Alloc> *m, T key, V def = V()) {
    return m ? get(*m, key, def) : def; }

template<class K, class T, class V, class Comp, class Alloc>
inline V *getref(std::map<K, V, Comp, Alloc> *m, T key) {
    return m ? getref(*m, key) : 0; }

template<class K, class T, class V, class Comp, class Alloc>
inline const V *getref(const std::map<K, V, Comp, Alloc> *m, T key) {
    return m ? getref(*m, key) : 0; }

}  // namespace GetImpl
using namespace GetImpl;  // NOLINT(build/namespaces)

/* iterate over the values in a map */
template<class PairIter>
class IterValues {
    class iterator : public std::iterator<
        typename std::iterator_traits<PairIter>::iterator_category,
        typename std::iterator_traits<PairIter>::value_type::second_type,
        typename std::iterator_traits<PairIter>::difference_type,
        typename std::iterator_traits<PairIter>::value_type::second_type*,
        typename std::iterator_traits<PairIter>::value_type::second_type&> {
        PairIter                it;
     public:
        explicit iterator(PairIter i) : it(i) {}
        iterator &operator++() { ++it; return *this; }
        iterator &operator--() { --it; return *this; }
        bool operator==(const iterator &i) const { return it == i.it; }
        bool operator!=(const iterator &i) const { return it != i.it; }
        decltype(*&it->second) operator*() const { return it->second; }
        decltype(&it->second) operator->() const { return &it->second; }
    } b, e;

 public:
        template<class U> IterValues(U &map) : b(map.begin()), e(map.end()) {}
        IterValues(PairIter b, PairIter e) : b(b), e(e) {}
        iterator begin() const { return b; }
        iterator end() const { return e; }
};

template<class Map> IterValues<typename Map::iterator>
Values(Map &m) { return IterValues<typename Map::iterator>(m); }

template<class Map> IterValues<typename Map::const_iterator>
Values(const Map &m) { return IterValues<typename Map::const_iterator>(m); }

template<class PairIter> IterValues<PairIter>
Values(std::pair<PairIter, PairIter> range) {
    return IterValues<PairIter>(range.first, range.second); }

#endif /* P4C_LIB_MAP_H_ */
