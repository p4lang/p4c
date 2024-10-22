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
