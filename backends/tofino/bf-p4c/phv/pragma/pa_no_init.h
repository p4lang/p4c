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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_INIT_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_INIT_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

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
    const PhvInfo &phv_i;

    /// List of fields for which the pragma pa_no_init has been specified
    /// Used to print logging messages
    ordered_set<const PHV::Field *> fields;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        fields.clear();
        return rv;
    }

    bool add_constraint(cstring field_name);

    bool preorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaNoInit(const PhvInfo &phv) : phv_i(phv) {}

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    /// @returns the set of fields fo which the pragma pa_no_init has been specified in the program
    const ordered_set<const PHV::Field *> &getFields() const { return fields; }
};

std::ostream &operator<<(std::ostream &out, const PragmaNoInit &pa_a);

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_INIT_H_ */
