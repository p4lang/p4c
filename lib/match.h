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

#ifndef _LIB_MATCH_H_
#define _LIB_MATCH_H_

#include <stdint.h>
#include <iostream>

/// The ternary expression being matched, given as a pair of bitmasks:
/// (word0, word1). The ternary expression cares about the value of an
/// input bit if the corresponding bit is set in exactly one of the
/// bitmasks. If it is set in word0, then the input must have a 0 in that
/// position. Otherwise, if it is set in word1, then the input must have a
/// 1 in that position. More formally, the ternary expression matches when
/// the following expression is true:
///     (input ^ word1) & (word0 ^ word1) == 0
struct match_t {
    uintmax_t   word0, word1;
    match_t() : word0(0), word1(0) {}
    match_t(uintmax_t w0, uintmax_t w1) : word0(w0), word1(w1) {}
    explicit operator bool() const { return (word0 | word1) != 0; }
    bool operator==(const match_t &a) const { return word0 == a.word0 && word1 == a.word1; }
    bool operator!=(const match_t &a) const { return word0 != a.word0 || word1 != a.word1; }
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
    static match_t dont_care(int size) { return match_t(size, 0, 0); }
};

std::ostream &operator <<(std::ostream &, match_t);
bool operator>>(const char *, match_t &);

#endif /*_LIB_MATCH_H_ */
