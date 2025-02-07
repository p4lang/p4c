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

#ifndef BACKENDS_TOFINO_BF_ASM_WIDEREG_H_
#define BACKENDS_TOFINO_BF_ASM_WIDEREG_H_

#include <limits.h>

#include <functional>
#include <iostream>
#include <sstream>

#include "lib/bitvec.h"
#include "lib/log.h"

using namespace P4;

void print_regname(std::ostream &out, const void *addr, const void *end);

struct widereg_base;

struct widereg_base {
    bitvec value, reset_value;
    mutable bool read, write;
    mutable bool disabled_;

    widereg_base() : read(false), write(false), disabled_(false) {}
    explicit widereg_base(bitvec v)
        : value(v), reset_value(v), read(false), write(false), disabled_(false) {}
    explicit widereg_base(uintptr_t v)
        : value(v), reset_value(v), read(false), write(false), disabled_(false) {}
#if __WORDSIZE == 64
    // For 32-bit systems intptr_t is defined as int
    explicit widereg_base(intptr_t v)
        : value(v), reset_value(v), read(false), write(false), disabled_(false) {}
#endif
    explicit widereg_base(int v)
        : value(v), reset_value(v), read(false), write(false), disabled_(false) {}
    operator bitvec() const {
        read = true;
        return value;
    }
    bool modified() const { return write; }
    void set_modified(bool v = true) { write = v; }
    bool disabled() const { return disabled_; }
    bool disable_if_unmodified() { return write ? false : (disabled_ = true); }
    bool disable_if_zero() const { return value.empty() && !write; }
    bool disable_if_reset_value() { return value == reset_value ? (disabled_ = true) : false; }
    bool disable() const {
        if (write) {
            LOG1("ERROR: Disabling modified register in " << this);
            return false;
        }
        disabled_ = true;
        return disabled_;
    }
    void enable() const { disabled_ = false; }
    void rewrite() { write = false; }
    virtual bitvec operator=(bitvec v) = 0;
    virtual unsigned size() = 0;
    void log(const char *op, bitvec v) const;
};

inline static unsigned int to_unsigned(const bitvec &v) {
    std::stringstream ss;
    ss << v;
    std::string str(ss.str());
    unsigned int rv = std::strtoul(str.c_str(), 0, 16);
    return rv;
}

inline std::ostream &operator<<(std::ostream &out, const widereg_base *u) {
    print_regname(out, u, u + 1);
    return out;
}
inline std::ostream &operator<<(std::ostream &out, const widereg_base &u) {
    return out << to_unsigned(u.value);
}

template <int N>
struct widereg : widereg_base {
    widereg() : widereg_base() {}
    const widereg &check() {
        if (value.max().index() >= N) {
            LOG1("ERROR: out of range for " << N << " bits in " << this);
            value.clrrange(N, value.max().index() - N + 1);
        }
        return *this;
    }
    explicit widereg(bitvec v) : widereg_base(v) { check(); }
    explicit widereg(uintptr_t v) : widereg_base(v) { check(); }
#if __WORDSIZE == 64
    // For 32-bit systems intptr_t is defined as int
    explicit widereg(intptr_t v) : widereg_base(v) { check(); }
#endif
    explicit widereg(int v) : widereg_base(v) { check(); }
    widereg(const widereg &) = delete;
    widereg(widereg &&) = default;
    bitvec operator=(bitvec v) {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (write) LOG1("WARNING: Overwriting " << value << " with " << v << " in " << this);
        value = v;
        write = true;
        log("=", v);
        check();
        return v;
    }
    uintptr_t operator=(uintptr_t v) {
        *this = bitvec(v);
        return v;
    }
    intptr_t operator=(intptr_t v) {
        *this = bitvec(v);
        return v;
    }
    const widereg &operator=(const widereg &v) {
        *this = v.value;
        v.read = true;
        return v;
    }
    const widereg_base &operator=(const widereg_base &v) {
        *this = v.value;
        v.read = true;
        return v;
    }
    unsigned size() { return N; }
    const widereg &operator|=(bitvec v) {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (write)
            LOG1("WARNING: Overwriting " << value << " with " << (v | value) << " in " << this);
        value |= v;
        write = true;
        log("|=", v);
        return check();
    }
    const widereg &set_subfield(uintptr_t v, unsigned bit, unsigned size) {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (bit + size > N) {
            LOG1("ERROR: subfield " << bit << ".." << (bit + size - 1) << " out of range in "
                                    << this);
        } else if (auto o = value.getrange(bit, size)) {
            if (write)
                LOG1((o != v ? "ERROR" : "WARNING")
                     << ": Overwriting subfield(" << bit << ".." << (bit + size - 1) << ") value "
                     << o << " with " << v << " in " << this);
        }
        if (v >= (1U << size))
            LOG1("ERROR: Subfield value " << v << " too large for " << size << " bits in " << this);
        value.putrange(bit, size, v);
        write = true;
        log("|=", bitvec(v) << bit);
        return check();
    }
};

#endif /* BACKENDS_TOFINO_BF_ASM_WIDEREG_H_ */
