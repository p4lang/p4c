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

#ifndef P4C_LIB_HEX_H_
#define P4C_LIB_HEX_H_

#include <iostream>
#include <iomanip>
#include <vector>

class hex {
    intmax_t    val;
    int         width;
    char        fill;
 public:
    explicit hex(intmax_t v, int w = 0, char f = ' ') : val(v), width(w), fill(f) {}
    explicit hex(void *v, int w = 0, char f = ' ') : val((intmax_t)v), width(w), fill(f) {}
    friend std::ostream &operator<<(std::ostream &os, const hex &h);
};

inline std::ostream &operator<<(std::ostream &os, const hex &h) {
    auto save = os.flags();
    auto save_fill = os.fill();
    os << std::hex << std::setw(h.width) << std::setfill(h.fill) << h.val;
    os.fill(save_fill);
    os.flags(save);
    return os; }

class hexvec {
    void        *data;
    size_t      elsize, len;
    int         width;
    char        fill;
 public:
    template<typename I> hexvec(I *d, size_t l, int w = 0, char f = ' ') :
        data(d), elsize(sizeof(I)), len(l), width(w), fill(f) {}
    template<typename T> hexvec(std::vector<T> &d, int w = 0, char f = ' ') :
        data(d.data()), elsize(sizeof(T)), len(d.size()), width(w), fill(f) {}
    friend std::ostream &operator<<(std::ostream &os, const hexvec &h);
};

std::ostream &operator<<(std::ostream &os, const hexvec &h);

#endif /* P4C_LIB_HEX_H_ */
