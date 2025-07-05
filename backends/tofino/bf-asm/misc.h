/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied.  See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_ASM_MISC_H_
#define BACKENDS_TOFINO_BF_ASM_MISC_H_

#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "asm-types.h"
#include "backends/tofino/bf-asm/json.h"

template <class T>
auto setup_muxctl(T &reg, int val) -> decltype((void)reg.enabled_2bit_muxctl_enable) {
    reg.enabled_2bit_muxctl_select = val;
    reg.enabled_2bit_muxctl_enable = 1;
}
template <class T>
auto setup_muxctl(T &reg, int val) -> decltype((void)reg.enabled_3bit_muxctl_enable) {
    reg.enabled_3bit_muxctl_select = val;
    reg.enabled_3bit_muxctl_enable = 1;
}
template <class T>
auto setup_muxctl(T &reg, int val) -> decltype((void)reg.enabled_4bit_muxctl_enable) {
    reg.enabled_4bit_muxctl_select = val;
    reg.enabled_4bit_muxctl_enable = 1;
}
template <class T>
auto setup_muxctl(T &reg, int val) -> decltype((void)reg.enabled_5bit_muxctl_enable) {
    reg.enabled_5bit_muxctl_select = val;
    reg.enabled_5bit_muxctl_enable = 1;
}
template <class T>
auto setup_muxctl(T &reg, int val) -> decltype((void)reg.exactmatch_row_vh_xbar_enable) {
    reg.exactmatch_row_vh_xbar_select = val;
    reg.exactmatch_row_vh_xbar_enable = 1;
}

template <class T, class Alloc>
void append(std::vector<T, Alloc> &a, const std::vector<T, Alloc> &b) {
    for (auto &e : b) a.push_back(e);
}

template <class T, class U>
T join(const std::vector<T> &vec, U sep) {
    T rv;
    bool first = true;
    for (auto &el : vec) {
        if (first)
            first = false;
        else
            rv += sep;
        rv += el;
    }
    return rv;
}

extern int remove_name_tail_range(std::string &, int *size = nullptr);

// Convert an integer to hex string of specified width (in bytes)
std::string int_to_hex_string(unsigned val, unsigned width);

// Add a reg to CJSON Configuration Cache
void add_cfg_reg(json::vector &cfg_cache, std::string full_name, std::string name, std::string val);

bool check_zero_string(const std::string &s);

// Get filename
std::string get_filename(const char *s);
std::string get_directory(const char *s);

/** Given a p4 name, eg. "inst.field", write "inst" to @instname and "field" to
 * @fieldname.  If @fullname cannot be split, writes @fullname to @instname and
 * "" to @fieldname.
 */
void gen_instfield_name(const std::string &fullname, std::string &instname, std::string &fieldname);

/// Compare pointers based on the pointed at type
/// For use as a Comparator for map/set types
template <class T>
struct ptrless {
    bool operator()(const T *a, const T *b) const { return b ? a ? *a < *b : true : false; }
    bool operator()(const std::unique_ptr<T> &a, const std::unique_ptr<T> &b) const {
        return b ? a ? *a < *b : true : false;
    }
};

/* word with size (lowest) bits set */
uint64_t bitMask(unsigned size);
/* word with range of bits from lo to hi (inclusive) set */
uint64_t bitRange(unsigned lo, unsigned hi);

int parity(uint32_t v);
int parity_2b(uint32_t v);  // two-bit parity (parity of pairs in the word)

inline bool check_value(const value_t value, const decltype(value_t::i) expected) {
    if (!CHECKTYPE(value, tINT)) return false;
    if (value.i != expected) {
        error(value.lineno, "unexpected value %ld; expected %ld", value.i, expected);
        return false;
    }
    return true;
}

/**
 * @brief Check range of an input integer value (tINT)
 *
 * This method is designated mainly for checking input integer constants. The template
 * parameter defines target type in which the value is going to be stored. As the
 * higher limit is quite often 0xffff... we must handle signed and unsigned integers
 * correctly.
 *
 * @tparam IntType Target type which the value will be stored in.
 * @param value The checked value
 * @param lo lower inclusive limit
 * @param hi higher include limit
 * @return False if the value is out of the specified limits
 */
template <typename IntType,
          typename = typename std::enable_if<std::is_integral<IntType>::value>::type>
bool check_range_strict(value_t value, IntType lo, IntType hi) {
    auto format_error_message([](value_t value, IntType lo, IntType hi) {
        /* -- As we don't know actual type of the IntType, we cannot use the printf-like
         *    formatting. */
        std::ostringstream oss;
        oss << "value " << value.i << " is out of allowed range <" << +lo << "; " << +hi << ">";
        error(value.lineno, "%s", oss.str().c_str());
    });

    if (!CHECKTYPE(value, tINT)) return false;

    /* -- Handle different ranges (signed, unsigned, different size) of the value_t::i
     *    and IntType. */

    if (!std::in_range<IntType>(value.i)) {
        format_error_message(value, lo, hi);
        return false;
    }

    /* -- Now check requested limits */
    IntType converted(static_cast<IntType>(value.i));
    if (converted < lo || converted > hi) {
        format_error_message(value, lo, hi);
        return false;
    }
    return true;
}

inline bool check_range(const value_t value, const decltype(value_t::i) lo,
                        const decltype(value_t::i) hi) {
    return check_range_strict<decltype(value_t::i)>(value, lo, hi);
}

inline bool check_range_match(const value_t &match, const decltype(match_t::word0) mask,
                              int width) {
    if (!CHECKTYPE(match, tMATCH)) return false;
    if ((match.m.word0 | match.m.word1) != mask) {
        error(match.lineno, "invalid match width; expected %i bits", width);
        return false;
    }
    return true;
}

template <typename IntType>
void convert_i2m(IntType i, match_t &m) {
    static_assert(sizeof(IntType) == sizeof(match_t::word0));
    static_assert(std::is_integral<IntType>::value);

    m.word0 = ~static_cast<decltype(match_t::word0)>(i);
    m.word1 = static_cast<decltype(match_t::word0)>(i);
}

bool check_bigint_unsigned(value_t value, uint32_t byte_width);

/// * is parsed as match_t::word0 == 0 && match_t::word1 == 0.
/// The function converts the match according to the specified with @p mask.
inline void fix_match_star(match_t &match, const decltype(match_t::word0) mask) {
    if (match.word0 == 0 && match.word1 == 0) match.word0 = match.word1 = mask;
}

/// The function reads a tINT or tMATCH value, performs range checks, and converts
/// the value to a new tMATCH value.
/// @param value Input value
/// @param match Output value
/// @param width Expected width of the input value used for range checks
/// @pre @p value must be a tINT or tMATCH value.
/// @return True if the value is correctly parsed
bool input_int_match(const value_t value, match_t &match, int width);

/// Check if a tMAP value contains all the given keys.
/// @param value A tMAP value
/// @param keys A set of keys
/// @pre @p value must be a tMAP
/// @return True if the given keys are a subset of the map's keys
bool require_keys(const value_t &data, std::set<const char *> keys);

#endif /* BACKENDS_TOFINO_BF_ASM_MISC_H_ */
