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

#ifndef BF_ASM_UBITS_H_  //  NOLINT(build/header_guard)
#define BF_ASM_UBITS_H_

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>

#include <functional>
#include <iostream>
#include <sstream>

#include "bitvec.h"
#include "log.h"

using namespace P4;

void declare_registers(const void *addr, size_t sz,
                       std::function<void(std::ostream &, const char *, const void *)> fn);
void undeclare_registers(const void *addr);
void print_regname(std::ostream &out, const void *addr, const void *end);
std::string string_regname(const void *addr, const void *end);

struct ubits_base;

struct ubits_base {
    uint64_t value, reset_value;
    mutable bool read, write;
    mutable bool disabled_;

    ubits_base() : value(0), reset_value(0), read(false), write(false), disabled_(false) {}
    explicit ubits_base(uint64_t v)
        : value(v), reset_value(v), read(false), write(false), disabled_(false) {}
    operator uint64_t() const {
        read = true;
        return value;
    }
    bool modified() const { return write; }
    void set_modified(bool v = true) { write = v; }
    bool disabled() const { return disabled_; }
    bool disable_if_unmodified() { return write ? false : (disabled_ = true); }
    bool disable_if_zero() const { return value == 0 && !write; }
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
    virtual uint64_t operator=(uint64_t v) = 0;
    virtual const ubits_base &operator|=(uint64_t v) = 0;
    virtual unsigned size() = 0;
    void log(const char *op, uint64_t v) const;
};

inline std::ostream &operator<<(std::ostream &out, const ubits_base *u) {
    print_regname(out, u, u + 1);
    return out;
}

template <int N>
struct ubits : ubits_base {
    ubits() : ubits_base() {}
    const ubits &check(std::true_type) {
        if (value >= (uint64_t(1) << N)) {
            LOG1("ERROR:  out of range for " << N << " bits in " << this);
            value &= (uint64_t(1) << N) - 1;
        }
        return *this;
    }
    const ubits &check(std::false_type) { return *this; }
    const ubits &check() {
        return check(std::integral_constant<bool, (N != sizeof(uint64_t) * CHAR_BIT)>{});
    }
    explicit ubits(uint64_t v) : ubits_base(v) { check(); }
    ubits(const ubits &) = delete;
    ubits(ubits &&) = default;
    uint64_t operator=(uint64_t v) override {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (write)
            LOG1((value != v ? "ERROR:" : "WARNING:")
                 << " Overwriting " << value << " with " << v << " in " << this);
        value = v;
        write = true;
        log("=", v);
        check();
        return v;
    }
    const ubits &operator=(const ubits &v) {
        *this = v.value;
        v.read = true;
        return v;
    }
    const ubits_base &operator=(const ubits_base &v) {
        *this = v.value;
        v.read = true;
        return v;
    }
    unsigned size() override { return N; }
    const ubits &operator|=(uint64_t v) override {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (write && (v & value) != 0)
            LOG1("WARNING: Overwriting " << value << " with " << (v | value) << " in " << this);
        value |= v;
        write = true;
        log("|=", v);
        return check();
    }
    const ubits &operator|=(bitvec v) {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (v.ffs(N) > 0)
            LOG1("ERROR: bitvec 0x" << v << " out of range for " << N << " bits in " << this);
        uint64_t val = v.getrange(0, N);
        if (write && (val & value) != 0)
            LOG1("WARNING: Overwriting " << value << " with " << (val | value) << " in " << this);
        value |= val;
        write = true;
        log("|=", val);
        return check();
    }
    const ubits &operator+=(uint64_t v) {
        if (disabled_) LOG1("ERROR: Overwriting disabled register value in " << this);
        value += v;
        write = true;
        log("+=", v);
        return check();
    }
    const ubits &operator^=(uint64_t v) {
        if (disabled_) LOG1("ERROR: Overwriting disabled register value in " << this);
        value ^= v;
        write = true;
        log("^=", v);
        return check();
    }
    const ubits &set_subfield(uint64_t v, unsigned bit, unsigned size) {
        if (disabled_) LOG1("ERROR: Overwriting disabled register value in " << this);
        uint64_t mask = (1ULL << size) - 1;
        uint64_t oldv = (value >> bit) & mask;
        if (bit + size > N) {
            LOG1("ERROR: subfield " << bit << ".." << (bit + size - 1) << " out of range in "
                                    << this);
        } else if (write && oldv) {
            LOG1((v != oldv ? "ERROR" : "WARNING")
                 << ": Overwriting subfield(" << bit << ".." << (bit + size - 1) << ") value "
                 << oldv << " with " << v << " in " << this);
        }
        if (v > mask) {
            LOG1("ERROR: Subfield value " << v << " too large for " << size << " bits in " << this);
            v &= mask;
        }
        value |= v << bit;
        write = true;
        log("|=", v << bit);
        return check();
    }
};

#endif /* BF_ASM_UBITS_H_ */
