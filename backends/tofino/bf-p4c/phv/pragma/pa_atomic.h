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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_ATOMIC_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_ATOMIC_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

using namespace P4;

/** Adds the atomic pragma to the specified fields with the specified gress.
 * Fields with this pragma cannot be split across containers.
 */
class PragmaAtomic : public Inspector {
    PhvInfo &phv_i;

    /// List of fields for which the pragma pa_atomic has been specified
    /// Used to print logging messages
    ordered_set<const PHV::Field *> fields;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        fields.clear();
        return rv;
    }

    bool add_constraint(const IR::BFN::Pipe *pipe, const IR::Expression *expr, cstring field_name);

    bool preorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaAtomic(PhvInfo &phv) : phv_i(phv) {}

    /// @returns the set of fields fo which the pragma pa_atomic has been specified in the program
    const ordered_set<const PHV::Field *> getFields() const { return fields; }

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;
};

std::ostream &operator<<(std::ostream &out, const PragmaAtomic &pa_a);

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_ATOMIC_H_ */
