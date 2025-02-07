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

#include "misc.h"

#include <regex>
#include <sstream>
#include <string>

#include "bfas.h"
#include "target.h"

int remove_name_tail_range(std::string &name, int *size) {
    auto tail = name.rfind('.');
    if (tail == std::string::npos) return 0;
    unsigned lo, hi;
    int len = -1;
    if (sscanf(&name[tail], ".%u-%u%n", &lo, &hi, &len) >= 2 && tail + len == name.size() &&
        hi >= lo) {
        name.erase(tail);
        if (size) *size = hi - lo + 1;
        return lo;
    }
    return 0;
}

std::string int_to_hex_string(unsigned val, unsigned width) {
    std::stringstream sval;
    sval << std::setfill('0') << std::setw(width) << std::hex << val << std::setfill(' ');
    return sval.str();
}

void add_cfg_reg(json::vector &cfg_cache, std::string full_name, std::string name,
                 std::string val) {
    json::map cfg_cache_reg;
    cfg_cache_reg["fully_qualified_name"] = full_name;
    cfg_cache_reg["name"] = name;
    cfg_cache_reg["value"] = val;
    cfg_cache.push_back(std::move(cfg_cache_reg));
}

bool check_zero_string(const std::string &s) {
    char zero = '0';
    return s.find_first_not_of(zero) == std::string::npos;
}

std::string get_filename(const char *s) {
    std::string fname = s;
    fname = fname.substr(fname.find_last_of("/") + 1);
    fname = fname.substr(0, fname.find_last_of("."));
    return fname;
}

std::string get_directory(const char *s) {
    std::string fname = s;
    auto tail = fname.find_last_of("/");
    if (tail == std::string::npos)
        fname = ".";
    else
        fname = fname.substr(0, tail);
    return fname;
}

/* Given a p4 name, split into instance and field names if possible
 *  - else return a copy of the original name */
void gen_instfield_name(const std::string &fullname, std::string &instname,
                        std::string &field_name) {
    auto dotpos = fullname.rfind('.');
    if (dotpos == std::string::npos) {
        instname = fullname;
        field_name = std::string();
    } else {
        instname = fullname.substr(0, dotpos);
        field_name = fullname.substr(dotpos + 1, fullname.size());
    }
}

uint64_t bitMask(unsigned size) {
    BUG_CHECK(size <= 64 && "bitMask(size), maximum size is 64");
    if (size == 64) return ~UINT64_C(0);
    return (UINT64_C(1) << size) - 1;
}

uint64_t bitRange(unsigned lo, unsigned hi) {
    BUG_CHECK(hi >= lo && hi < 64, "bitRange(%u,%u) invalid", lo, hi);
    if (lo == 0 && hi + 1 == 64) return ~UINT64_C(0);
    return ((UINT64_C(1) << (hi - lo + 1)) - 1) << lo;
}

int parity(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return v & 1;
}

int parity_2b(uint32_t v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v ^= v >> 2;
    return v & 3;
}

bool check_bigint_unsigned(value_t value, uint32_t byte_width) {
    BUG_CHECK(value.type == tBIGINT);

    /* -- zero is in the range */
    if (value.bigi.size == 0) return true;

    constexpr uint64_t size_bigint_item(sizeof(value.bigi.data[0]));

    bool overflow(false);

    /* -- all items above the max_index must by zero */
    const uint64_t max_index(((byte_width + size_bigint_item - 1) / size_bigint_item) - 1);
    for (int i(max_index + 1); i < value.bigi.size; ++i) {
        if (value.bigi.data[i] != 0) {
            overflow = true;
        }
    }
    /* -- check limit in the boundary bigint part */
    if (value.bigi.size > max_index) {
        const uint64_t ext_width(byte_width % size_bigint_item);
        if (ext_width > 0 && value.bigi.data[max_index] >= (1 << (ext_width * 8))) {
            overflow = true;
        }
    }
    if (overflow) {
        error(value.lineno, "the integer constant is wider than the requested width %u bytes",
              byte_width);
        return false;
    }

    return true;
}

bool input_int_match(const value_t value, match_t &match, int width) {
    BUG_CHECK(width <= sizeof(match_t::word0) * 8);

    using MatchType = decltype(match_t::word0);
    MatchType mask;
    if (width < sizeof(MatchType) * 8)
        mask = (1ULL << width) - 1;
    else
        mask = std::numeric_limits<MatchType>::max();
    if (value.type == tINT) {
        if (!check_range_strict<MatchType>(value, 0, mask)) return false;
        convert_i2m(value.i, match);
    } else if (value.type == tBIGINT) {
        /* -- As the match type is uint64_t and value_t::i is int64_t, constants
         *    above 0x7fffffffffffffff are passed as big integers. */
        if (value.bigi.size > 1) {
            error(value.lineno, "the match constant is out of the expected range <0, %lu>", mask);
            return false;
        }
        MatchType v(0);
        if (value.bigi.size > 0) v = value.bigi.data[0];
        if (v > mask) {
            error(value.lineno, "the match constant is out of the expected range <0, %lu>", mask);
            return false;
        }
        convert_i2m(v, match);
    } else {
        value_t fixed_value = value;
        fixed_value.m = value.m;
        fix_match_star(fixed_value.m, mask);
        if (!check_range_match(fixed_value, mask, width)) return false;
        match = fixed_value.m;
    }
    return true;
}

unsigned match_t::dirtcam(unsigned width, unsigned bit) {
    static unsigned masks[] = {0x5555, 0x3333, 0xf0f0, 0xffff};
    BUG_CHECK(width <= 4, "dirtcam of more than 4 bits?");
    unsigned rv = (1U << (1U << width)) - 1;
    for (unsigned i = 0; i < width; ++i, ++bit) {
        if (!((word0 >> bit) & 1)) rv &= ~masks[i];
        if (!((word1 >> bit) & 1)) rv &= masks[i];
    }
    return rv;
}

unsigned wmatch_t::dirtcam(unsigned width, unsigned bit) {
    static unsigned masks[] = {0x5555, 0x3333, 0xf0f0, 0xffff};
    BUG_CHECK(width <= 4, "dirtcam of more than 4 bits?");
    unsigned rv = (1U << (1U << width)) - 1;
    for (unsigned i = 0; i < width; ++i, ++bit) {
        // treat both bits 0 as don't care rather than never match
        if (!word0[bit] && !word1[bit]) continue;
        if (!word0[bit]) rv &= ~masks[i];
        if (!word1[bit]) rv &= masks[i];
    }
    return rv;
}

bool require_keys(const value_t &data, std::set<const char *> keys) {
    for (auto key : keys) {
        pair_t *kv = data.map[key];
        if (!kv) {
            error(data.lineno, "missing required key '%s'", key);
            return false;
        }
    }
    return true;
}
