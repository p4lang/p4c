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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_PACK_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_PACK_H_

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

using namespace P4;

class PragmaNoPack : public Inspector {
    const PhvInfo &phv_i;
    std::vector<std::pair<const PHV::Field *, const PHV::Field *>> no_packs_i;
    const ordered_map<cstring, ordered_set<cstring>> &no_pack_constr;
    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        no_packs_i.clear();
        return rv;
    }

    bool preorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaNoPack(const PhvInfo &phv)
        : phv_i(phv), no_pack_constr(*(new ordered_map<cstring, ordered_set<cstring>>)) {}
    PragmaNoPack(const PhvInfo &phv,
                 const ordered_map<cstring, ordered_set<cstring>> &no_pack_constr)
        : phv_i(phv), no_pack_constr(no_pack_constr) {}
    const std::vector<std::pair<const PHV::Field *, const PHV::Field *>> &no_packs() const {
        return no_packs_i;
    }

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_PACK_H_ */
