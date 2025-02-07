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

#include "ubits.h"

#include <map>
#include <sstream>

#include "hex.h"
#include "log.h"

struct regrange {
    const char *base;
    size_t sz;
    std::function<void(std::ostream &, const char *, const void *)> fn;
};

static std::map<const char *, regrange> *registers;

static regrange *find_regrange(const void *addr_) {
    const char *addr = static_cast<const char *>(addr_);
    if (registers) {
        auto it = registers->upper_bound(addr);
        if (it != registers->begin()) {
            it--;
            if (addr <= it->second.base + it->second.sz) return &it->second;
        }
    }
    return nullptr;
}

void declare_registers(const void *addr_, size_t sz,
                       std::function<void(std::ostream &, const char *, const void *)> fn) {
    const char *addr = static_cast<const char *>(addr_);
    if (!registers) registers = new std::map<const char *, regrange>();
    registers->emplace(addr, regrange{addr, sz, fn});
}

void undeclare_registers(const void *addr_) {
    const char *addr = static_cast<const char *>(addr_);
    registers->erase(addr);
    if (registers->empty()) {
        delete registers;
        registers = 0;
    }
}

void print_regname(std::ostream &out, const void *addr, const void *end) {
    if (auto rr = find_regrange(addr))
        rr->fn(out, static_cast<const char *>(addr), end);
    else
        out << "???";
}

std::string string_regname(const void *addr, const void *end) {
    std::stringstream tmp;
    print_regname(tmp, addr, end);
    return tmp.str();
}

void ubits_base::log(const char *op, uint64_t v) const {
    if (LOGGING(1)) {
        std::ostringstream tmp;
        if (!find_regrange(this)) return;
        LOG1(this << ' ' << op << ' ' << v
                  << (v != value ? tmp << " (now " << value << ")", tmp : tmp).str() << " (0x"
                  << hex(value) << ")");
    }
}
