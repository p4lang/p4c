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

#include <sstream>
#include <string>

#ifndef BF_ASM_MATCH_SOURCE_H_
#define BF_ASM_MATCH_SOURCE_H_

/**
 * A source for a match key of a table.  The source can either be from the input xbar, or from the
 * galois field matrix, as indicated in uArch Section Exact Match Row Vertical/Horizontal (VH)
 * Xbars.  This class is the parent of HashMatchSource and Phv::Ref.
 */
class MatchSource : public IHasDbPrint {
 public:
    virtual int fieldlobit() const = 0;
    virtual int fieldhibit() const = 0;
    virtual unsigned size() const = 0;
    virtual int slicelobit() const = 0;
    virtual int slicehibit() const = 0;
    virtual const char *name() const = 0;
    virtual int get_lineno() const = 0;
    virtual std::string toString() const = 0;
    virtual void dbprint(std::ostream &out) const = 0;
};

/**
 * The source used by proxy hash tables for their match key.
 */
class HashMatchSource : public MatchSource {
    int lo = 0;
    int hi = 0;

 public:
    int lineno = 0;
    HashMatchSource(int line, int l, int h) : lo(l), hi(h), lineno(line) {}
    explicit HashMatchSource(value_t value) {
        if (CHECKTYPE(value, tCMD)) {
            lineno = value.lineno;
            if (value != "hash_group")
                error(value.lineno, "Hash Match source must come from a hash group");
            if (value.vec.size != 2) error(value.lineno, "Hash Match source requires a range");
            if (CHECKTYPE(value.vec[1], tRANGE)) {
                lo = value.vec[1].lo;
                hi = value.vec[1].hi;
            }
        }
    }

    int get_lineno() const override { return lineno; }
    int fieldlobit() const override { return lo < 0 ? 0 : lo; }
    int fieldhibit() const override { return hi < 0 ? 0 : hi; }
    unsigned size() const override { return hi >= lo && lo >= 0 ? hi - lo + 1 : 0; }
    int slicelobit() const override { return fieldlobit(); }
    int slicehibit() const override { return fieldhibit(); }
    const char *name() const override { return "hash_group"; }
    std::string toString() const override {
        std::stringstream str;
        str << *this;
        return str.str();
    }

    void dbprint(std::ostream &out) const { out << name() << "(" << lo << ".." << hi << ")"; }
};

#endif /* BF_ASM_MATCH_SOURCE_H_ */
