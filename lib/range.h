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
std::ostream &operator<<(std::ostream &out, const RangeIter<T> &r) {
    out << r.cur;
    if (r.cur + r.incr != r.fin)
        out << ".." << (r.fin - r.incr);
    return out;
}

#endif /* _LIB_RANGE_H_ */
