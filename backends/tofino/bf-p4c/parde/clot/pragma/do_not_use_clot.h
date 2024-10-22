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

#ifndef BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_PRAGMA_DO_NOT_USE_CLOT_H_
#define BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_PRAGMA_DO_NOT_USE_CLOT_H_

#include <map>
#include <optional>

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

using namespace P4;

/**
 * @brief do_not_use_clot pragma support.
 *
 * This pass will gather all do_not_use_clot pragmas and generate do_not_use_clot:
 * a set of fields that should not be allocated to CLOTs.
 */
class PragmaDoNotUseClot : public Inspector {
    const PhvInfo &phv_info;
    ordered_set<const PHV::Field *> do_not_use_clot;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        do_not_use_clot.clear();
        return rv;
    }

    /**
     * @brief Get global pragma do_not_use_clot.
     */
    bool preorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaDoNotUseClot(const PhvInfo &phv_info) : phv_info(phv_info) {}

    // BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    const ordered_set<const PHV::Field *> &do_not_use_clot_fields() const {
        return do_not_use_clot;
    }
};

std::ostream &operator<<(std::ostream &out, const PragmaDoNotUseClot &do_not_use_clot);

#endif /* BACKENDS_TOFINO_BF_P4C_PARDE_CLOT_PRAGMA_DO_NOT_USE_CLOT_H_ */
