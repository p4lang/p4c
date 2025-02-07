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

#ifndef BACKENDS_TOFINO_BF_ASM_HASHEXPR_H_
#define BACKENDS_TOFINO_BF_ASM_HASHEXPR_H_

#include "backends/tofino/bf-utils/dynamic_hash/dynamic_hash.h"
#include "input_xbar.h"
#include "phv.h"

class HashExpr : public IHasDbPrint {
    class PhvRef;
    class Random;
    class Crc;
    class XorHash;
    class Xor;
    class Mask;
    class Stripe;
    class Slice;
    class SExtend;

 protected:
    explicit HashExpr(int l) : lineno(l) {}

 public:
    int lineno;
    bfn_hash_algorithm_t hash_algorithm = {};  // Zero-init to make Klockwork happy
    static HashExpr *create(gress_t, int stage, const value_t &);
    virtual void build_algorithm() = 0;
    virtual bool check_ixbar(InputXbar *ix, InputXbar::HashTable ht) = 0;
    virtual void gen_data(bitvec &data, int bit, InputXbar *ix, InputXbar::HashTable hash_table);
    void gen_ixbar_init(ixbar_init_t *ixbar_init, std::vector<ixbar_input_t> &inputs,
                        std::vector<hash_matrix_output_t> &outputs, int logical_hash_bit,
                        InputXbar *ix, InputXbar::HashTable hash_table);
    virtual void gen_ixbar_inputs(std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                  InputXbar::HashTable hash_table) = 0;
    virtual void get_sources(int bit, std::vector<Phv::Ref> &) const = 0;
    std::vector<Phv::Ref> get_sources(int bit) const {
        std::vector<Phv::Ref> rv;
        get_sources(bit, rv);
        return rv;
    }
    virtual int width() = 0;
    virtual int input_size() = 0;
    virtual bool match_phvref(const Phv::Ref &ref) { return false; }
    virtual bool operator==(const HashExpr &) const = 0;
    void find_input(Phv::Ref what, std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                    InputXbar::HashTable hash_table);
    bool operator!=(const HashExpr &a) const { return !operator==(a); }
    virtual void dbprint(std::ostream &out) const {}
    virtual Phv::Ref *get_ghost_slice() { return nullptr; }
    virtual ~HashExpr() {}

 private:
    void generate_ixbar_inputs_with_gaps(const std::multimap<unsigned, Phv::Ref> &what,
                                         std::vector<ixbar_input_t> &inputs, InputXbar *ix,
                                         InputXbar::HashTable hash_table);
};

extern void dump(const HashExpr *);
extern void dump(const HashExpr &);

#endif /* BACKENDS_TOFINO_BF_ASM_HASHEXPR_H_ */
