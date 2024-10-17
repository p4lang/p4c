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

#ifndef EXTENSIONS_BF_P4C_PARDE_CLOT_PRAGMA_DO_NOT_USE_CLOT_H_
#define EXTENSIONS_BF_P4C_PARDE_CLOT_PRAGMA_DO_NOT_USE_CLOT_H_

#include <map>
#include <optional>
#include "ir/ir.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"

using namespace P4;

/**
 * @brief do_not_use_clot pragma support.
 *
 * This pass will gather all do_not_use_clot pragmas and generate do_not_use_clot:
 * a set of fields that should not be allocated to CLOTs.
 */
class PragmaDoNotUseClot : public Inspector {
    const PhvInfo& phv_info;
    ordered_set<const PHV::Field*> do_not_use_clot;

    profile_t init_apply(const IR::Node* root) override {
        profile_t rv = Inspector::init_apply(root);
        do_not_use_clot.clear();
        return rv;
    }

    /**
     * @brief Get global pragma do_not_use_clot.
     */
    bool preorder(const IR::BFN::Pipe* pipe) override;

 public:
    explicit PragmaDoNotUseClot(const PhvInfo& phv_info) : phv_info(phv_info) { }

    // BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    const ordered_set<const PHV::Field*>& do_not_use_clot_fields() const {
        return do_not_use_clot;
    }
};

std::ostream& operator<<(std::ostream& out, const PragmaDoNotUseClot& do_not_use_clot);

#endif /* EXTENSIONS_BF_P4C_PARDE_CLOT_PRAGMA_DO_NOT_USE_CLOT_H_ */
