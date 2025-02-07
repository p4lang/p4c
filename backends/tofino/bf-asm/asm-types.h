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

#ifndef BF_ASM_ASM_TYPES_H_
#define BF_ASM_ASM_TYPES_H_

#include <assert.h>
#include <bitops.h>
#include <bitvec.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>

#include "bfas.h"
#include "json.h"
#include "map.h"
#include "mask_counter.h"
#include "vector.h"

enum gress_t { INGRESS, EGRESS, GHOST, NUM_GRESS_T };

/* All timing related uses combine the INGRESS and GHOST threads (they run in lockstep), so
 * we remap GHOST->INGRESS when dealing with timing */
inline gress_t timing_thread(gress_t gress) { return gress == GHOST ? INGRESS : gress; }
/* imem similarly shares color between INGRESS and GHOST */
inline gress_t imem_thread(gress_t gress) { return gress == GHOST ? INGRESS : gress; }

struct match_t {
    uint64_t word0, word1;
#ifdef __cplusplus
    operator bool() const { return (word0 | word1) != 0; }
    bool operator==(const match_t &a) const { return word0 == a.word0 && word1 == a.word1; }
    bool matches(uint64_t v) const {
        return (v | word1) == word1 && ((~v & word1) | word0) == word0;
    }
    bool matches(const match_t &v) const {
        assert(0);
        return false;
    }
    unsigned dirtcam(unsigned width, unsigned bit);
#endif /* __cplusplus */
};

DECLARE_VECTOR(match_t);

struct wmatch_t {
    bitvec word0, word1;
#ifdef __cplusplus
    wmatch_t() = default;
    wmatch_t(const wmatch_t &) = default;
    wmatch_t(wmatch_t &&) = default;
    wmatch_t &operator=(const wmatch_t &) = default;
    wmatch_t &operator=(wmatch_t &&) = default;
    wmatch_t(const match_t &v) : word0(v.word0), word1(v.word1) {}  // NOLINT(runtime/explicit)
    wmatch_t(const VECTOR(match_t) & v) {                           // NOLINT(runtime/explicit)
        for (int i = 0; i < v.size; ++i) {
            word0.putrange(i * 64, 64, v.data[i].word0);
            word1.putrange(i * 64, 64, v.data[i].word1);
        }
    }
    operator bool() const { return word0 || word1; }
    bool operator==(const wmatch_t &a) const { return word0 == a.word0 && word1 == a.word1; }
    bool matches(bitvec v) const { return (v | word1) == word1 && ((word1 - v) | word0) == word0; }
    bool matches(const wmatch_t &v) const {
        assert(0);
        return false;
    }
    unsigned dirtcam(unsigned width, unsigned bit);
#endif /* __cplusplus */
};

enum value_type { tINT, tBIGINT, tRANGE, tSTR, tMATCH, tBIGMATCH, tVEC, tMAP, tCMD };
extern const char *value_type_desc[];

struct value_t;
struct pair_t;
#ifdef __cplusplus
DECLARE_VECTOR(
    value_t, value_t &operator[](int) const; value_t & back() const;
    value_t * begin() const { return data; } value_t * end() const; value_t & front() const;
    VECTOR(value_t) & add(value_t &&); VECTOR(value_t) & add(int);
    VECTOR(value_t) & add(const char *);)
DECLARE_VECTOR(
    pair_t, void push_back(const char *, value_t &&);  // NOLINT(whitespace/operators)
    pair_t & operator[](int) const; pair_t * operator[](const char *) const; pair_t & back() const;
    pair_t * begin() const { return data; } pair_t * end() const; pair_t & front() const;)
#else
DECLARE_VECTOR(value_t)
DECLARE_VECTOR(pair_t)
#endif /* __cplusplus */
DECLARE_VECTOR(uintptr_t);

struct value_t {
    enum value_type type;
    int lineno;
    union {
        int64_t i;
        VECTOR(uintptr_t) bigi;
        struct {
            int lo;
            int hi;
        };
        char *s;
        match_t m;
        VECTOR(match_t) bigm;
        VECTOR(value_t) vec;
        VECTOR(pair_t) map;
    };
#ifdef __cplusplus
    value_t &operator[](int i) const {
        assert(type == tVEC || type == tCMD);
        return vec[i];
    }
    bool startsWith(const char *pfx) const {
        if (type == tSTR) return strncmp(s, pfx, strlen(pfx)) == 0;
        if (type == tCMD && vec.size > 0 && vec[0].type == tSTR)
            return strncmp(vec[0].s, pfx, strlen(pfx)) == 0;
        return false;
    }
    bool checkSize() const {
        if (type == tVEC) return (vec.size > 0);
        if (type == tMAP) return (map.size > 0);
        if (type == tCMD) return (vec.size > 0);
        return true;
    }
#endif /* __cplusplus */
};

struct pair_t {
    struct value_t key, value;
#ifdef __cplusplus
    pair_t() = default;
    pair_t(const value_t &k, const value_t &v) : key(k), value(v) {}
#endif /* __cplusplus */
};

void free_value(value_t *p);
const char *value_desc(const value_t *v);
static inline void free_pair(pair_t *p) {
    free_value(&p->key);
    free_value(&p->value);
}
bool get_bool(const value_t &v);

// If max_bits is zero, no testing or masking is carried out.
// If error_message is set, values larger than max_bits will error, otherwise the value is masked.
bitvec get_bitvec(const value_t &v, unsigned max_bits = 0, const char *error_message = nullptr);
uint64_t get_int64(const value_t &v, unsigned max_bits = 0, const char *error_message = nullptr);

#ifdef __cplusplus
bool operator==(const struct value_t &, const struct value_t &);
inline bool operator==(const struct value_t &a, const char *b) {
    if (a.type == tCMD && a.vec.size > 0 && a[0].type == tSTR) return !strcmp(a[0].s, b);
    return a.type == tSTR && !strcmp(a.s, b);
}
inline bool operator==(const char *a, const struct value_t &b) {
    if (b.type == tCMD && b.vec.size > 0 && b[0].type == tSTR) return !strcmp(a, b[0].s);
    return b.type == tSTR && !strcmp(a, b.s);
}
inline bool operator==(const struct value_t &a, int b) { return a.type == tINT && a.i == b; }
inline bool operator==(int a, const struct value_t &b) { return b.type == tINT && a == b.i; }

inline const char *value_desc(const value_t &v) { return value_desc(&v); }

template <class A, class B>
inline bool operator!=(A a, B b) {
    return !(a == b);
}

inline value_t &VECTOR(value_t)::operator[](int i) const {
    assert(i >= 0 && i < size);
    return data[i];
}
inline pair_t &VECTOR(pair_t)::operator[](int i) const {
    assert(i >= 0 && i < size);
    return data[i];
}
inline pair_t *VECTOR(pair_t)::operator[](const char *k) const {
    for (int i = 0; i < size; i++)
        if (data[i].key == k) return &data[i];
    return 0;
}
inline value_t *VECTOR(value_t)::end() const { return data + size; }
inline value_t &VECTOR(value_t)::front() const {
    assert(0 < size);
    return data[0];
}
inline value_t &VECTOR(value_t)::back() const {
    assert(0 < size);
    return data[size - 1];
}
inline pair_t *VECTOR(pair_t)::end() const { return data + size; }
inline pair_t &VECTOR(pair_t)::front() const {
    assert(0 < size);
    return data[0];
}
inline pair_t &VECTOR(pair_t)::back() const {
    assert(0 < size);
    return data[size - 1];
}

/* can't call VECTOR(pair_t)::push_back directly except from the compilation unit where
 * it is defined, due to gcc bug.  Workaround via global function */
extern void push_back(VECTOR(pair_t) & m, const char *s,
                      value_t &&v);  // NOLINT(whitespace/operators)

inline void fini(value_t &v) { free_value(&v); }
inline void fini(pair_t &p) { free_pair(&p); }
inline void fini(VECTOR(value_t) & v) {
    VECTOR_foreach(v, free_value);
    VECTOR_fini(v);
}
inline void fini(VECTOR(pair_t) & v) {
    VECTOR_foreach(v, free_pair);
    VECTOR_fini(v);
}
void collapse_list_of_maps(value_t &, bool singleton_only = false);

std::unique_ptr<json::obj> toJson(value_t &);
std::unique_ptr<json::vector> toJson(VECTOR(value_t) &);
std::unique_ptr<json::map> toJson(pair_t &);
std::unique_ptr<json::map> toJson(VECTOR(pair_t) &);

#endif /* __cplusplus */

#define CHECKTYPE(V, T) \
    ((V).type == (T) || (error((V).lineno, "Syntax error, expecting %s", value_type_desc[T]), 0))
#define CHECKTYPESIZE(V, T) \
    (CHECKTYPE(V, T) &&     \
     ((V).checkSize() || (error((V).lineno, "Syntax error, empty %s", value_type_desc[T]), 0)))
#define PCHECKTYPE(P, V, T)      \
    (((P) && (V).type == (T)) || \
     (error((V).lineno, "Syntax error, expecting %s", value_type_desc[T]), 0))
#define CHECKTYPEM(V, T, M) \
    ((V).type == (T) || (error((V).lineno, "Syntax error, expecting %s", M), 0))
#define CHECKTYPEPM(V, T, P, M) \
    (((V).type == (T) && (P)) || (error((V).lineno, "Syntax error, expecting %s", M), 0))
#define PCHECKTYPEM(P, V, T, M) \
    (((P) && (V).type == (T)) || (error((V).lineno, "Syntax error, expecting %s", M), 0))
#define CHECKTYPE2(V, T1, T2)                                                               \
    ((V).type == (T1) || (V).type == (T2) ||                                                \
     (error((V).lineno, "Syntax error, expecting %s or %s but got %s", value_type_desc[T1], \
            value_type_desc[T2], value_desc(V)),                                            \
      0))
#define CHECKTYPE3(V, T1, T2, T3)                                                      \
    ((V).type == (T1) || (V).type == (T2) || (V).type == (T3) ||                       \
     (error((V).lineno, "Syntax error, expecting %s or %s or %s", value_type_desc[T1], \
            value_type_desc[T2], value_type_desc[T3]),                                 \
      0))
#define PCHECKTYPE2(P, V, T1, T2)                                                \
    (((P) && ((V).type == (T1) || (V).type == (T2))) ||                          \
     (error((V).lineno, "Syntax error, expecting %s or %s", value_type_desc[T1], \
            value_type_desc[T2]),                                                \
      0))
#define CHECKTYPE2M(V, T1, T2, M)            \
    ((V).type == (T1) || (V).type == (T2) || \
     (error((V).lineno, "Syntax error, expecting %s but got %s", M, value_desc(V)), 0))
#define PCHECKTYPE2M(P, V, T1, T2, M)                   \
    (((P) && ((V).type == (T1) || (V).type == (T2))) || \
     (error((V).lineno, "Syntax error, expecting %s", M), 0))
#define VALIDATE_RANGE(V)                      \
    ((V).type != tRANGE || (V).lo <= (V).hi || \
     (error((V).lineno, "Invalid range %d..%d", (V).lo, (V).hi), 0))

inline value_t *get(VECTOR(pair_t) & map, const char *key) {
    for (auto &kv : map)
        if (kv.key == key) return &kv.value;
    return 0;
}
inline const value_t *get(const VECTOR(pair_t) & map, const char *key) {
    for (auto &kv : map)
        if (kv.key == key) return &kv.value;
    return 0;
}

#ifdef __cplusplus

template <class T>
inline void parse_vector(std::vector<T> &vec, const VECTOR(value_t) & data) {
    for (auto &v : data) vec.emplace_back(v);
}
template <>
inline void parse_vector(std::vector<int> &vec, const VECTOR(value_t) & data) {
    for (auto &v : data)
        if (CHECKTYPE(v, tINT)) vec.push_back(v.i);
}
template <>
inline void parse_vector(std::vector<int64_t> &vec, const VECTOR(value_t) & data) {
    for (auto &v : data)
        if (CHECKTYPE(v, tINT)) vec.push_back(v.i);
}
template <>
inline void parse_vector(std::vector<std::string> &vec, const VECTOR(value_t) & data) {
    for (auto &v : data)
        if (CHECKTYPE(v, tSTR)) vec.emplace_back(v.s);
}
template <class T>
inline void parse_vector(std::vector<T> &vec, const value_t &data) {
    if (data.type == tVEC)
        parse_vector(vec, data.vec);
    else
        vec.emplace_back(data);
}
template <>
inline void parse_vector(std::vector<int> &vec, const value_t &data) {
    if (CHECKTYPE2(data, tINT, tVEC)) {
        if (data.type == tVEC)
            parse_vector(vec, data.vec);
        else
            vec.push_back(data.i);
    }
}
template <>
inline void parse_vector(std::vector<int64_t> &vec, const value_t &data) {
    if (CHECKTYPE2(data, tINT, tVEC)) {
        if (data.type == tVEC)
            parse_vector(vec, data.vec);
        else
            vec.push_back(data.i);
    }
}
template <>
inline void parse_vector(std::vector<std::string> &vec, const value_t &data) {
    if (CHECKTYPE2(data, tSTR, tVEC)) {
        if (data.type == tVEC)
            parse_vector(vec, data.vec);
        else
            vec.push_back(data.s);
    }
}

std::ostream &operator<<(std::ostream &out, match_t m);
void print_match(FILE *fp, match_t m);

inline std::ostream &operator<<(std::ostream &out, gress_t gress) {
    switch (gress) {
        case INGRESS:
            out << "ingress";
            break;
        case EGRESS:
            out << "egress";
            break;
        case GHOST:
            out << "ghost";
            break;
        default:
            out << "(invalid gress " << static_cast<int>(gress) << ")";
    }
    return out;
}

template <typename T>
inline std::string to_string(T val) {
    std::stringstream tmp;
    tmp << val;
    return tmp.str();
}

class MapIterChecked {
    /* Iterate through a map (VECTOR(pair_t)), giving errors for non-string and
     * duplicate keys (and skipping them) */
    const VECTOR(pair_t) & map;
    bool allow;  // allow non-string keys
    std::set<std::string> duplicates_allowed;
    std::map<std::string, int> keys_seen;
    class iter {
        MapIterChecked *self;
        pair_t *p;
        void check() {
            while (p != self->map.end()) {
                if (self->allow && p->key.type != tSTR) break;
                if (!CHECKTYPE(p->key, tSTR)) {
                    p++;
                    continue;
                }
                if (self->duplicates_allowed.count(p->key.s)) break;
                if (self->keys_seen.count(p->key.s)) {
                    error(p->key.lineno, "Duplicate element %s", p->key.s);
                    warning(self->keys_seen[p->key.s], "previous element %s", p->key.s);
                    p++;
                    continue;
                }
                self->keys_seen[p->key.s] = p->key.lineno;
                break;
            }
        }

     public:
        iter(MapIterChecked *s, pair_t *p_) : self(s), p(p_) { check(); }
        pair_t &operator*() const { return *p; }
        pair_t *operator->() const { return p; }
        bool operator==(iter &a) const { return p == a.p; }
        iter &operator++() {
            p++;
            check();
            return *this;
        }
    };

 public:
    explicit MapIterChecked(const VECTOR(pair_t) & map_, bool o = false,
                            const std::set<std::string> &dup = {})
        : map(map_), allow(o), duplicates_allowed(dup) {}
    MapIterChecked(const VECTOR(pair_t) & map_, const std::set<std::string> &dup)
        : map(map_), allow(false), duplicates_allowed(dup) {}
    iter begin() { return iter(this, map.begin()); }
    iter end() { return iter(this, map.end()); }
};

class MatchIter {
    /* Iterate through the integers that match a match_t */
    match_t m;
    class iter : public MaskCounter {
        MatchIter *self;

     public:
        explicit iter(MatchIter *s) : MaskCounter(s->m.word0 & s->m.word1), self(s) {
            if (!(self->m.word1 | self->m.word0)) overflow();
        }
        unsigned operator*() const {
            return this->operator unsigned() | (self->m.word1 & ~self->m.word0);
        }
        iter &end() {
            overflow();
            return *this;
        }
    };

 public:
    explicit MatchIter(match_t m_) : m(m_) {}
    iter begin() { return iter(this); }
    iter end() { return iter(this).end(); }
};

class SrcInfo {
    int lineno;
    friend std::ostream &operator<<(std::ostream &, const SrcInfo &);

 public:
    explicit SrcInfo(int l) : lineno(l) {}
};

struct RegisterSetBase {
    virtual ~RegisterSetBase() = default;
};

struct ParserRegisterSet : public RegisterSetBase {};

/// An interface for parsing a section of a .bfa file
class Parsable {
 public:
    /// @param data entire map/sequence of elements
    virtual void input(VECTOR(value_t) args, value_t data) = 0;
    virtual ~Parsable() = default;
};

/// An interface for writing into registers
class Configurable {
 public:
    virtual void write_config(RegisterSetBase &regs, json::map &json, bool legacy = true) = 0;
    virtual ~Configurable() = default;
};

/// An interface for generating context.json
class Contextable {
 public:
    virtual void output(json::map &ctxtJson) = 0;
    virtual ~Contextable() = default;
};

#endif /* __cplusplus */

#endif /* BF_ASM_ASM_TYPES_H_ */
