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

#ifndef LIB_ALGORITHM_H_
#define LIB_ALGORITHM_H_

#include <algorithm>
#include <set>

// Round up x/y, for integers
#define ROUNDUP(x, y) (((x) + (y)-1) / (y))
// Elements in an array
#define ELEMENTS(a) (sizeof(a) / sizeof(a[0]))

/* these should all be in <algorithm>, but are missing... */

template <class C, class T>
inline bool contains(C &c, const T &val) {
    return std::find(c.begin(), c.end(), val) != c.end();
}

template <class C, class Pred>
inline bool contains_if(C &c, Pred pred) {
    return std::find_if(c.begin(), c.end(), pred) != c.end();
}

template <class C, class Pred>
inline void erase_if(C &c, Pred pred) {
    for (auto it = c.begin(); it != c.end();) {
        if (pred(*it))
            it = c.erase(it);
        else
            ++it;
    }
}

template <class C, class Pred>
inline void remove_if(C &c, Pred pred) {
    c.erase(std::remove_if(c.begin(), c.end(), pred), c.end());
}

template <class C, class T>
inline typename C::iterator find(C &c, const T &val) {
    return std::find(c.begin(), c.end(), val);
}

using std::max_element;
using std::min_element;

template <class C>
inline typename C::const_iterator min_element(const C &c) {
    return min_element(c.begin(), c.end());
}
template <class C, class Compare>
inline typename C::const_iterator min_element(const C &c, Compare comp) {
    return min_element(c.begin(), c.end(), comp);
}

template <class C>
inline typename C::const_iterator max_element(const C &c) {
    return max_element(c.begin(), c.end());
}
template <class C, class Compare>
inline typename C::const_iterator max_element(const C &c, Compare comp) {
    return max_element(c.begin(), c.end(), comp);
}

template <class Iter, class Fn>
inline Fn for_each(std::pair<Iter, Iter> range, Fn fn) {
    return std::for_each(range.first, range.second, fn);
}

template <class Iter>
Iter begin(std::pair<Iter, Iter> pr) {
    return pr.first;
}
template <class Iter>
Iter end(std::pair<Iter, Iter> pr) {
    return pr.second;
}

#endif /* LIB_ALGORITHM_H_ */
