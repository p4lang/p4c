#ifndef LIB_SET_H_
#define LIB_SET_H_

#include <set>

/* stuff that should be in std::set but is missing... */

template<class T, class C1, class A1, class U> inline
auto operator|=(std::set<T, C1, A1> &a, U &b) -> decltype(b.begin(), a) {
    for (auto &el : b) a.insert(el);
    return a; }
template<class T, class C1, class A1, class U> inline
auto operator-=(std::set<T, C1, A1> &a, U &b) -> decltype(b.begin(), a) {
    for (auto &el : b) a.erase(el);
    return a; }
template<class T, class C1, class A1, class U> inline
auto operator&=(std::set<T, C1, A1> &a, U &b) -> decltype(b.begin(), a) {
    for (auto it = a.begin(); it != a.end();) {
        if (b.count(*it))
            ++it;
        else
            it = a.erase(it); }
    return a; }

template<class T, class C1, class A1, class U> inline
auto contains(std::set<T, C1, A1> &a, U &b) -> decltype(b.begin(), true) {
    for (auto &el : b) if (!a.count(el)) return false;
    return true; }
template<class T, class C1, class A1, class U> inline
auto intersects(std::set<T, C1, A1> &a, U &b) -> decltype(b.begin(), true) {
    for (auto &el : b) if (a.count(el)) return true;
    return false; }

#endif /* LIB_SET_H_ */
