/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BF_P4C_MAU_GATEWAY_H_
#define BF_P4C_MAU_GATEWAY_H_

#include <set>

#include "bf-p4c/device.h"
#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "lib/safe_vector.h"

using namespace P4;

namespace PHV {
class Field;
}  // namespace PHV

class PhvInfo;

struct Device::GatewaySpec {
    int PhvBytes;
    int HashBits;
    int PredicateBits;
    int MaxRows;
    bool SupportXor;
    bool SupportRange;
    int ExactShifts;
    bool ByteSwizzle;  // is the a byte swizzle between ixbar and gateway
    int PerByteMatch;  // lower bytes are shared per row, with 1 bit match per row
    unsigned XorByteSlots;
};

class CanonGatewayExpr : public MauTransform {
 public:
    CanonGatewayExpr() : _debugIndent(6) {}

 private:
    IR::MAU::Action *preorder(IR::MAU::Action *af) override {
        prune();
        return af;
    }
    IR::P4Table *preorder(IR::P4Table *t) override {
        prune();
        return t;
    }
    IR::Attached *preorder(IR::Attached *a) override {
        prune();
        return a;
    }
    IR::Node *preorder(IR::MAU::Table *) override;

    /// Use preorder visitors to simplify all the expressions that have reduntant terms
    /// before going into other more complicated rules (deMorgan, distributions, etc.)
    const IR::Expression *preorder(IR::LAnd *) override;
    const IR::Expression *preorder(IR::LOr *) override;
    const IR::Expression *preorder(IR::LNot *) override;

    const IR::Expression *postorder(IR::Operation::Relation *) override;
    const IR::Expression *postorder(IR::Leq *) override;
    const IR::Expression *postorder(IR::Lss *) override;
    const IR::Expression *postorder(IR::Geq *) override;
    const IR::Expression *postorder(IR::Grt *) override;
    const IR::Expression *postorder(IR::LAnd *) override;
    const IR::Expression *postorder(IR::LOr *) override;
    const IR::Expression *postorder(IR::LNot *) override;
    const IR::Expression *postorder(IR::BAnd *) override;
    const IR::Expression *postorder(IR::BOr *) override;
    const IR::Expression *postorder(IR::MAU::TypedPrimitive *) override;
    const IR::Node *postorder(IR::MAU::Table *) override;
    // helper functions
    using GWRow_t = std::pair<const IR::Expression *, cstring>;
    void removeUnusedRows(IR::MAU::Table *, bool isCanon);
    void sortGatewayRows(safe_vector<GWRow_t> &gateway_rows);
    void splitGatewayRows(safe_vector<GWRow_t> &gateway_rows);
    void removeNotEquals(safe_vector<GWRow_t> &gateway_rows);
    class NeedNegate;

 private:
    /// indentation for debugging nested invocations of postorder methods
    indent_t _debugIndent;
};

class CollectGatewayFields : public Inspector {
    const PhvInfo &phv;
    const IXBar::Use *ixbar = nullptr;
    const IR::MAU::Table *tbl = nullptr;
    unsigned row_limit = ~0U;  // FIXME -- needed?  only use by SplitComplexGateways
    PHV::FieldSlice xor_match;
    bool preorder(const IR::MAU::Table *tbl) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::MAU::TypedPrimitive *) override;
    void postorder(const IR::Literal *) override;
    void postorder(const IR::Operation::Relation *) override { xor_match = {}; }

 public:
    struct info_t {
        ordered_set<PHV::FieldSlice> xor_with;  // {x: x ==/!= this field in gateway }
        bool const_eq = false;                  // bits compared ==/!= const
        bool need_range = false;                // bits needed in range compares
        bool valid_bit = false;                 // TOFINO1-ONLY: implicit container valid bit
        safe_vector<std::pair<int, le_bitrange>> offsets;
        safe_vector<std::pair<int, le_bitrange>> xor_offsets;
    };
    ordered_map<PHV::FieldSlice, info_t> info;
    ordered_map<const info_t *, cstring> info_to_uses;
    bool need_range = false;
    int bytes = 0, bits = 0;
    explicit CollectGatewayFields(const PhvInfo &phv, const IXBar::Use *ix = nullptr)
        : phv(phv), ixbar(ix) {}
    CollectGatewayFields(const PhvInfo &phv, unsigned rl) : phv(phv), row_limit(rl) {}
    bool compute_offsets();
    friend std::ostream &operator<<(std::ostream &, const info_t &);
    friend std::ostream &operator<<(std::ostream &, const CollectGatewayFields &);
};

class CollectMatchFieldsAsGateway : public CollectGatewayFields {
    bool fail;
    bool preorder(const IR::MAU::Table *tbl) override;

 public:
    explicit CollectMatchFieldsAsGateway(const PhvInfo &phv, const IXBar::Use *ixb = nullptr)
        : CollectGatewayFields(phv, ixb), fail(false) {}
    explicit operator bool() { return !fail; }
};

class GatewayRangeMatch : public MauModifier {
    const PhvInfo &phv;
    void postorder(IR::MAU::Table *) override;
    // optimization -- prune parts of the tree we know can't contain MAU::Tables
    bool preorder(IR::MAU::Action *) override { return false; }
    bool preorder(IR::P4Table *) override { return false; }
    bool preorder(IR::Attached *) override { return false; }
    class SetupRanges;

 public:
    explicit GatewayRangeMatch(const PhvInfo &phv) : phv(phv) {}
};

class CheckGatewayExpr : public MauInspector {
    const PhvInfo &phv;
    bool needConstOperand(const IR::Operation::Binary *);
    bool preorder(const IR::MAU::Table *tbl) override;
    bool preorder(const IR::MAU::Action *) override { return false; }
    bool preorder(const IR::MAU::TableKey *) override { return false; }
    bool preorder(const IR::P4Table *) override { return false; }
    bool preorder(const IR::Attached *) override { return false; }
    bool preorder(const IR::MAU::BackendAttached *) override { return false; }
    bool preorder(const IR::Equ *) override { return true; }
    bool preorder(const IR::Neq *) override { return true; }
    bool preorder(const IR::BAnd *e) override { return needConstOperand(e); }
    bool preorder(const IR::BOr *e) override { return needConstOperand(e); }
    bool preorder(const IR::LAnd *) override { return true; }
    bool preorder(const IR::LOr *) override { return true; }
    bool preorder(const IR::LNot *) override { return true; }
    bool preorder(const IR::Literal *) override { return true; }
    bool preorder(const IR::RangeMatch *) override { return true; }
    bool preorder(const IR::Expression *e) override;
    bool preorder(const IR::Operation::Relation *rel) override { return needConstOperand(rel); }

 public:
    explicit CheckGatewayExpr(const PhvInfo &phv) : phv(phv) {}
};

class BuildGatewayMatch : public Inspector {
    const PhvInfo &phv;
    CollectGatewayFields &fields;
    match_t match;
    safe_vector<int> range_match;
    std::map<int, match_t> byte_matches;
    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::MAU::TypedPrimitive *) override;
    bool preorder(const IR::LAnd *) override { return true; }
    bool preorder(const IR::LNot *) override { return true; }
    bool preorder(const IR::BAnd *) override { return true; }
    bool preorder(const IR::BOr *) override { return true; }
    void constant(big_int val);
    bool preorder(const IR::Constant *c) override {
        constant(c->value);
        return true;
    }
    bool preorder(const IR::BoolLiteral *c) override {
        constant(c->value);
        return true;
    }
    bool preorder(const IR::Equ *) override;
    bool preorder(const IR::Neq *) override;
    bool preorder(const IR::RangeMatch *) override;
    friend std::ostream &operator<<(std::ostream &, const BuildGatewayMatch &);
    PHV::FieldSlice match_field;
    big_int andmask = 0, ormask = 0, cmplmask = 0;
    int shift = 0, maxbit = 0;
    bool check_per_byte_match(const std::pair<int, le_bitrange> &byte, big_int mask, big_int val);

 public:
    BuildGatewayMatch(const PhvInfo &phv, CollectGatewayFields &f);
};

class GatewayOpt : public PassManager {
 public:
    explicit GatewayOpt(const PhvInfo &);
};

#endif /* BF_P4C_MAU_GATEWAY_H_ */
