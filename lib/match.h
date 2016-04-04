#ifndef _LIB_MATCH_H_
#define _LIB_MATCH_H_

#include <stdint.h>
#include <iostream>

struct match_t {
    uintmax_t   word0, word1;
    match_t() : word0(0), word1(0) {}
    match_t(uintmax_t w0, uintmax_t w1) : word0(w0), word1(w1) {}
    explicit operator bool() const { return (word0 | word1) != 0; }
    bool operator==(const match_t &a) const { return word0 == a.word0 && word1 == a.word1; }
    bool matches(uintmax_t v) const {
        return (v | word1) == word1 && ((~v & word1) | word0) == word0; }
    void setwidth(int bits) {
        uintmax_t mask = ~(~uintmax_t(0) << bits);
        word0 &= mask;
        word1 &= mask;
        mask &= ~(word0 | word1);
        word0 |= mask;
        word1 |= mask; }
    match_t(int size, uintmax_t val, uintmax_t mask) : word0(~val&mask), word1(val&mask)
        { setwidth(size); }
};

std::ostream &operator <<(std::ostream &, match_t);

#endif /*_LIB_MATCH_H_ */
