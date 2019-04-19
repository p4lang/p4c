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

#ifndef _LIB_RANGE_H_
#define _LIB_RANGE_H_

#include <iostream>

template<class T> class RangeIter {
    int incr;
    T cur, fin;
    explicit RangeIter(T end) : incr(0), cur(end), fin(end) {}

 public:
    RangeIter(T start, T end)
    : incr(start <= end ? 1 : -1), cur(start), fin(static_cast<T>(end + incr)) {}
    RangeIter begin() const { return *this; }
    RangeIter end() const { return RangeIter(fin); }
    T operator*() const { return cur; }
    bool operator==(const RangeIter &a) const { return cur == a.cur; }
    bool operator!=(const RangeIter &a) const { return cur != a.cur; }
    RangeIter &operator++() { cur = static_cast<T>(cur + incr); return *this; }
    template<class U> friend std::ostream &operator<<(std::ostream &, const RangeIter<U> &);
};

template<class T>
static inline RangeIter<T> Range(T a, T b) { return RangeIter<T>(a, b); }
template<class T>
static inline RangeIter<T> Range(std::pair<T, T> p) { return RangeIter<T>(p.first, p.second); }
template<class T>
static inline RangeIter<T> ReverseRange(std::pair<T, T> p) {
    return RangeIter<T>(p.second, p.first); }

template<class T>
std::ostream &operator<<(std::ostream &out, const RangeIter<T> &r) {
    out << r.cur;
    if (r.cur + r.incr != r.fin)
        out << ".." << (r.fin - r.incr);
    return out;
}

#endif /* _LIB_RANGE_H_ */
