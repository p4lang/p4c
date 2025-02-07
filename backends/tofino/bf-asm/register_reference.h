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

#ifndef BF_ASM_REGISTER_REFERENCE_H_
#define BF_ASM_REGISTER_REFERENCE_H_

#include <functional>
#include <iostream>

#include "log.h"

/* used by `dump_unread` methods to hold a concatenation of string literals for printing.
 * Allocated on the stack, the `pfx` chain prints the calling context */
struct prefix {
    const prefix *pfx;
    const char *str;  // should always be a string literal
    prefix(const prefix *p, const char *s) : pfx(p), str(s) {}
};

inline std::ostream &operator<<(std::ostream &out, const prefix *p) {
    if (p) {
        if (p->pfx) out << p->pfx << '.';
        out << p->str;
    }
    return out;
}

/* Class to link register trees together into a larger dag that will expand into a tree
 * when dumped as binary (so trees that appear in mulitple places will be duplicated)
 * 'name' is the json file name to use when dumping as cfg.json, and the name for logging
 * 'tree' is the subtree to dump as binary at the appropriate offset
 */
template <class REG>
class register_reference {
    REG *tree = nullptr;
    std::string name;

 public:
    mutable bool read = false, write = false, disabled_ = false;
    register_reference() {}
    register_reference(const register_reference &) = default;
    register_reference(register_reference &&) = default;
    register_reference &operator=(const register_reference &) & = default;
    register_reference &operator=(register_reference &&) & = default;
    ~register_reference() {}

    register_reference &set(const char *a, REG *r) {
        if (disabled_) LOG1("ERROR: Writing disabled register value in " << this);
        if (write) LOG1("WARNING: Overwriting \"" << name << "\" with \"" << a << "\" in " << this);
        name = a;
        tree = r;
        log();
        write = true;
        return *this;
    }
    const char *c_str() const { return name.c_str(); }
    REG *operator->() const {
        read = true;
        return tree;
    }
    explicit operator bool() const { return tree != nullptr; }
    bool modified() const { return write; }
    void set_modified(bool v = true) { write = v; }
    void rewrite() { write = false; }
    // friend std::ostream &operator<<(std::ostream &out, const register_reference<REG> &u);
    void enable() { disabled_ = false; }
    bool disabled() const { return disabled_; }
    bool disable_if_unmodified() { return false; }
    bool disable_if_zero() { return false; }
    bool disable_if_reset_value() { return false; }
    bool disable() {
        if (!name.empty()) {
            LOG1("ERROR: Disabling modified register in " << this);
            return false;
        }
        tree = nullptr;
        disabled_ = true;
        return true;
    }
    void log() const { LOG1(this << " = \"" << name << "\""); }
};

template <class REG>
inline std::ostream &operator<<(std::ostream &out, const register_reference<REG> *u) {
    print_regname(out, u, u + 1);
    return out;
}
template <class REG>
inline std::ostream &operator<<(std::ostream &out, const register_reference<REG> &u) {
    if (!*u.c_str())
        out << 0;
    else
        out << '"' << u.c_str() << '"';
    return out;
}

#endif /* BF_ASM_REGISTER_REFERENCE_H_ */
