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

#ifndef EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_NO_INIT_H_
#define EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_NO_INIT_H_

#include "ir/ir.h"
#include "bf-p4c/phv/phv_fields.h"

using namespace P4;

/**
 * @ingroup LowerParser
 * @brief Adds the no_init pragma to the specified fields with the specified gress.
 *
 * Specifies that the indicated metadata field does not require initialization to 0 before its
 * first use because the control plane guarantees that, in cases where this field's value is
 * needed, it will always be written before it is read.
 */
class PragmaNoInit : public Inspector {
    const PhvInfo& phv_i;

    /// List of fields for which the pragma pa_no_init has been specified
    /// Used to print logging messages
    ordered_set<const PHV::Field*> fields;

    profile_t init_apply(const IR::Node* root) override {
        profile_t rv = Inspector::init_apply(root);
        fields.clear();
        return rv;
    }

    bool add_constraint(cstring field_name);

    bool preorder(const IR::BFN::Pipe* pipe) override;

 public:
    explicit PragmaNoInit(const PhvInfo& phv) : phv_i(phv) { }

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    /// @returns the set of fields fo which the pragma pa_no_init has been specified in the program
    const ordered_set<const PHV::Field*>& getFields() const {
        return fields;
    }
};

std::ostream& operator<<(std::ostream& out, const PragmaNoInit& pa_a);

#endif /* EXTENSIONS_BF_P4C_PHV_PRAGMA_PA_NO_INIT_H_ */
