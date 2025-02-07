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

#include "asm-types.h"

#include <assert.h>
#include <stdlib.h>

#include "misc.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
void VECTOR(pair_t)::push_back(const char *s, value_t &&v) {  // NOLINT(whitespace/operators)
    pair_t entry{{tSTR, v.lineno}, v};
    entry.key.s = strdup(s);
    VECTOR_push(*this, entry);
    memset(&v, 0, sizeof(v));
}

void push_back(VECTOR(pair_t) & m, const char *s, value_t &&v) {  // NOLINT(whitespace/operators)
    m.push_back(s, std::move(v));
}

VECTOR(value_t) & VECTOR(value_t)::add(value_t &&v) {
    VECTOR_add(*this, std::move(v));
    return *this;
}
VECTOR(value_t) & VECTOR(value_t)::add(int v) {
    value_t tmp{tINT, v};
    VECTOR_add(*this, tmp);
    return *this;
}
VECTOR(value_t) & VECTOR(value_t)::add(const char *v) {
    value_t tmp{tSTR, -1};
    tmp.s = const_cast<char *>(v);
    VECTOR_add(*this, tmp);
    return *this;
}

/** check a value and see if it is a list of maps -- if so, concatenate the
 * maps into a single map and replace the list with that */
void collapse_list_of_maps(value_t &v, bool singleton_only) {
    if (v.type != tVEC || v.vec.size == 0) return;
    for (int i = 0; i < v.vec.size; i++) {
        if (v[i].type != tMAP) return;
        if (singleton_only && v[i].map.size != 1) return;
    }
    VECTOR(pair_t) map = v[0].map;
    for (int i = 1; i < v.vec.size; i++) {
        VECTOR_addcopy(map, v[i].map.data, v[i].map.size);
        VECTOR_fini(v[i].map);
    }
    VECTOR_fini(v.vec);
    v.type = tMAP;
    v.map = map;
}

std::unique_ptr<json::obj> toJson(value_t &v) {
    switch (v.type) {
        case tINT:
            return json::mkuniq<json::number>(v.i);
        case tBIGINT:
            if (v.bigi.size == 1 && v.bigi.data[0] < INT64_MAX)
                return json::mkuniq<json::number>(v.bigi.data[0]);
            // fall through
        case tRANGE:
        case tMATCH:
            return json::mkuniq<json::string>(value_desc(v));
        case tSTR:
            if (v == "true") return json::mkuniq<json::True>();
            if (v == "false") return json::mkuniq<json::False>();
            if (v == "null") return std::unique_ptr<json::obj>();
            return json::mkuniq<json::string>(v.s);
        case tVEC:
            return toJson(v.vec);
        case tMAP:
            return toJson(v.map);
        case tCMD:
            return toJson(v.vec);
        default:
            assert(0);
    }
    return std::unique_ptr<json::obj>();
}

std::unique_ptr<json::vector> toJson(VECTOR(value_t) & v) {
    auto rv = json::mkuniq<json::vector>();
    auto &vec = *rv;
    for (auto &el : v) vec.push_back(toJson(el));
    return rv;
}

std::unique_ptr<json::map> toJson(pair_t &kv) {
    auto rv = json::mkuniq<json::map>();
    auto &map = *rv;
    map[toJson(kv.key)] = toJson(kv.value);
    return rv;
}

std::unique_ptr<json::map> toJson(VECTOR(pair_t) & m) {
    auto rv = json::mkuniq<json::map>();
    auto &map = *rv;
    for (auto &kv : m) map[toJson(kv.key)] = toJson(kv.value);
    return rv;
}

bool get_bool(const value_t &v) {
    if (v == "true")
        return true;
    else if (v == "false")
        return false;
    else if (CHECKTYPE(v, tINT))
        return v.i != 0;
    return false;
}

bitvec get_bitvec(const value_t &v, unsigned max_bits, const char *error_message) {
    bitvec bv;
    if (CHECKTYPE2(v, tINT, tBIGINT)) {
        if (v.type == tINT) {
            bv.setraw(v.i);
        } else {
            if (!v.bigi.size) return bv;
            bv.setraw(v.bigi.data, v.bigi.size);
        }
    }
    if (!max_bits) return bv;
    int bits = bv.max().index() + 1;
    if (error_message && bits > max_bits) error(v.lineno, "%s", error_message);
    bv.clrrange(max_bits, bits);
    return bv;
}

uint64_t get_int64(const value_t &v, unsigned max_bits, const char *error_message) {
    BUG_CHECK(max_bits <= 64);
    bool too_large = false;
    uint64_t value = 0;
    if (CHECKTYPE2(v, tINT, tBIGINT)) {
        if (v.type == tINT) {
            value = (uint64_t)v.i;
        } else {
            if (!v.bigi.size) return 0;
            if (sizeof(uintptr_t) == sizeof(uint32_t)) {
                value = ((uint64_t)v.bigi.data[1] << 32) + v.bigi.data[0];
                too_large = v.bigi.size > 2;
            } else {
                BUG_CHECK(sizeof(uintptr_t) == sizeof(uint64_t));
                value = v.bigi.data[0];
                too_large = v.bigi.size > 1;
            }
        }
    }
    if (!max_bits) return value;
    uint64_t masked = value;
    if (max_bits < 64) masked &= (1ULL << max_bits) - 1;
    if (error_message && (too_large || masked != value)) error(v.lineno, "%s", error_message);
    return masked;
}

static int chkmask(const match_t &m, int maskbits) {
    uint64_t mask = bitMask(maskbits);
    int shift = 0;
    while (mask && ((m.word0 | m.word1) >> shift)) {
        if ((mask & m.word0 & m.word1) && (mask & m.word0 & m.word1) != mask) return -1;
        mask <<= maskbits;
        shift += maskbits;
    }
    return shift - maskbits;
}

std::ostream &operator<<(std::ostream &out, match_t m) {
    int shift, bits;
    if ((shift = chkmask(m, (bits = 4))) >= 0)
        out << "0x";
    else if ((shift = chkmask(m, (bits = 3))) >= 0)
        out << "0o";
    else if ((shift = chkmask(m, (bits = 1))) >= 0)
        out << "0b";
    else if ((shift = chkmask(m, (bits = 0))) == 0)
        out << "0b*";
    else
        assert(0);
    uint64_t mask = bitMask(bits) << shift;
    for (; mask; shift -= bits, mask >>= bits)
        if (mask & m.word0 & m.word1)
            out << '*';
        else
            out << "0123456789abcdef"[(m.word1 & mask) >> shift];
    return out;
}

void print_match(FILE *fp, match_t m) {
    std::stringstream tmp;
    tmp << m;
    fputs(tmp.str().c_str(), fp);
}

const char *value_type_desc[] = {"integer",    "bigint",           "range",
                                 "identifier", "match pattern",    "big match",
                                 "list",       "key: value pairs", "operation"};

const char *value_desc(const value_t *p) {
    static char buffer[32];
    switch (p->type) {
        case tINT:
            snprintf(buffer, sizeof(buffer), "%" PRId64 "", p->i);
            return buffer;
        case tBIGINT:
            return "<bigint>";
        case tRANGE:
            snprintf(buffer, sizeof(buffer), "%d..%d", p->lo, p->hi);
            return buffer;
        case tMATCH:
            return "<pattern>";
        case tBIGMATCH:
            return "<bigmatch>";
        case tSTR:
            return p->s;
        case tVEC:
            return "<list>";
        case tMAP:
            return "<map>";
        case tCMD:
            if (p->vec.size > 0 && p->vec.data[0].type == tSTR) return p->vec.data[0].s;
            return "<cmd>";
    }
    assert(false && "unknown value type");
    return "";
}

void free_value(value_t *p) {
    switch (p->type) {
        case tBIGINT:
            VECTOR_fini(p->bigi);
            break;
        case tSTR:
            free(p->s);
            break;
        case tVEC:
        case tCMD:
            VECTOR_foreach(p->vec, free_value);
            VECTOR_fini(p->vec);
            break;
        case tMAP:
            VECTOR_foreach(p->map, free_pair);
            VECTOR_fini(p->map);
            break;
        default:
            break;
    }
}

bool operator==(const struct value_t &a, const struct value_t &b) {
    int i;
    if (a.type != b.type) {
        if (a.type == tINT && b.type == tBIGINT) {
            if (a.i < 0 || (size_t)a.i != b.bigi.data[0]) return false;
            for (i = 1; i < b.bigi.size; i++)
                if (b.bigi.data[i]) return false;
            return true;
        } else if (a.type == tBIGINT && b.type == tINT) {
            if (b.i < 0 || (size_t)b.i != a.bigi.data[0]) return false;
            for (i = 1; i < a.bigi.size; i++)
                if (a.bigi.data[i]) return false;
            return true;
        }
        return false;
    }
    switch (a.type) {
        case tINT:
            return a.i == b.i;
        case tBIGINT:
            for (i = 0; i < a.bigi.size && i < b.bigi.size; i++)
                if (a.bigi.data[i] != b.bigi.data[i]) return false;
            for (; i < a.bigi.size; i++)
                if (a.bigi.data[i]) return false;
            for (; i < b.bigi.size; i++)
                if (b.bigi.data[i]) return false;
            return true;
        case tRANGE:
            return a.lo == b.lo && a.hi == b.hi;
        case tSTR:
            return !strcmp(a.s, b.s);
        case tMATCH:
            return a.m.word0 == b.m.word0 && a.m.word1 == b.m.word1;
        case tVEC:
        case tCMD:
            if (a.vec.size != b.vec.size) return false;
            for (int i = 0; i < a.vec.size; i++)
                if (a.vec.data[i] != b.vec.data[i]) return false;
            return true;
        case tMAP:
            if (a.map.size != b.map.size) return false;
            for (int i = 0; i < a.map.size; i++) {
                if (a.map.data[i].key != b.map.data[i].key) return false;
                if (a.map.data[i].value != b.map.data[i].value) return false;
            }
            return true;
    }
    assert(false && "unknown value type");
    return "";
}
#pragma GCC diagnostic pop
