/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BF_P4C_MAU_DYNHASH_H_
#define BF_P4C_MAU_DYNHASH_H_

#include <vector>
#include <boost/range/irange.hpp>
#include "mau_visitor.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/json.h"
#include "lib/ordered_map.h"
#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

struct HashFuncLoc {
    int stage;
    int hash_group;

    bool operator<(const HashFuncLoc &hfl) const {
        if (stage != hfl.stage)
            return stage < hfl.stage;
        return hash_group < hfl.hash_group;
    }

    bool operator==(const HashFuncLoc &hfl) const {
        return stage == hfl.stage && hash_group != hfl.hash_group;
    }

    bool operator!=(const HashFuncLoc &hfl) const {
        return !(*this == hfl);
    }

    HashFuncLoc(int s, int hg) : stage(s), hash_group(hg) {}
};

using NameToHashGen = std::map<cstring, const IR::MAU::HashGenExpression *>;
using HashGenToAlloc
    = std::map<cstring, safe_vector<std::pair<HashFuncLoc, const Tofino::IXBar::HashDistIRUse *>>>;
using AllocToHashUse = std::map<HashFuncLoc, safe_vector<const Tofino::IXBar::HashDistIRUse *>>;

class VerifyUniqueDynamicHash : public MauInspector {
    NameToHashGen &verify_hash_gen;

    profile_t init_apply(const IR::Node *node) override {
        auto rv = MauInspector::init_apply(node);
        verify_hash_gen.clear();
        return rv;
    }

    bool preorder(const IR::MAU::HashGenExpression *) override;
 public:
    explicit VerifyUniqueDynamicHash(NameToHashGen &vhg) : verify_hash_gen(vhg) {}
};

class GatherDynamicHashAlloc : public MauInspector {
    const NameToHashGen &verify_hash_gen;
    HashGenToAlloc &hash_gen_alloc;

    profile_t init_apply(const IR::Node *node) override {
        auto rv = MauInspector::init_apply(node);
        hash_gen_alloc.clear();
        return rv;
    }

    bool preorder(const IR::MAU::Table *) override;

 public:
    GatherDynamicHashAlloc(const NameToHashGen &vhg, HashGenToAlloc &hga)
        : verify_hash_gen(vhg), hash_gen_alloc(hga) {}
};


namespace BFN {

static const unsigned fieldListHandleBase = (0x21 << 24);
// dynamic hash handle base is 0x8, not 0x22
static const unsigned dynHashHandleBase = (0x8 << 24);
static const unsigned algoHandleBase = (0x23 << 24);
extern unsigned fieldListHandle;
extern unsigned dynHashHandle;
extern unsigned algoHandle;
static std::map<cstring, unsigned> algoHandles;
class GenerateDynamicHashJson : public MauInspector {
 private:
    const PhvInfo &phv;
    Util::JsonArray *_dynHashNode;
    const NameToHashGen &verify_hash_gen;
    const HashGenToAlloc &hash_gen_alloc;
    bool all_placed = true;

    bool preorder(const IR::MAU::Table *tbl) override;
    void gen_ixbar_json(const IXBar::Use *ixbar_use,
        Util::JsonObject *_dhc, int stage, const cstring field_list_name,
        const IR::NameList *algorithms, int hash_width = -1);

    void end_apply() override;

    void gen_ixbar_bytes_json(Util::JsonArray *_xbar_cfgs, int stage,
        const std::map<cstring, cstring> &fieldNames, const IXBar::Use &ixbar_use);
    void gen_algo_json(Util::JsonObject *_dhc, const IR::MAU::HashGenExpression *hge);
    void gen_single_algo_json(Util::JsonArray *_algos, const IR::MAU::HashFunction *alg,
        cstring alg_name, bool &is_default);
    void gen_hash_json(Util::JsonArray *_hash_cfgs, int stage, Tofino::IXBar::Use &ixbar_use,
        int &hash_bit_width);
    void gen_field_list_json(Util::JsonObject *_field_list, const IR::MAU::HashGenExpression *hge,
        std::map<cstring, cstring> &fieldNames);
    void gen_hash_dist_json(cstring dyn_hash_name);

 public:
    GenerateDynamicHashJson(const PhvInfo &p, Util::JsonArray *_dhn, const NameToHashGen &vhg,
            const HashGenToAlloc &hga)
        : phv(p), _dynHashNode(_dhn), verify_hash_gen(vhg), hash_gen_alloc(hga) {}
    /// output the json hierarchy into the asm file (as Yaml)
};


class DynamicHashJson : public PassManager {
    const PhvInfo &phv;
    Util::JsonArray *_dynHashNode = nullptr;
    NameToHashGen verify_hash_gen;
    HashGenToAlloc hash_gen_alloc;

    friend std::ostream & operator<<(std::ostream &out, const DynamicHashJson &dyn);

 public:
    explicit DynamicHashJson(const PhvInfo &p) : phv(p) {
        _dynHashNode = new Util::JsonArray();
        addPasses({
            new VerifyUniqueDynamicHash(verify_hash_gen),
            new GatherDynamicHashAlloc(verify_hash_gen, hash_gen_alloc),
            new GenerateDynamicHashJson(phv, _dynHashNode, verify_hash_gen, hash_gen_alloc)
        });
    }
};

}  // namespace BFN

#endif  /* BF_P4C_MAU_DYNHASH_H_ */
