#ifndef P4C_LIB_MAP_H_
#define P4C_LIB_MAP_H_

#include <map>

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
