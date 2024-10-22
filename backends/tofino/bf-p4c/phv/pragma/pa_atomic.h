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
