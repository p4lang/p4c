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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_MUTUALLY_EXCLUSIVE_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_MUTUALLY_EXCLUSIVE_H_

#include <map>
#include <optional>

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

using namespace P4;

/** pa_mutually_exclusive pragma support.
 *
 * This pass will gather all pa_mutually_exclusive pragmas and generate pa_mutually_exclusive_i:
 * mapping from a field to the set of fields it is mutually exclusive with.
 *
 */
class PragmaMutuallyExclusive : public Inspector {
    const PhvInfo &phv_i;
    ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> pa_mutually_exclusive_i;
    ordered_map<cstring, ordered_set<cstring>> mutually_exclusive_headers;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        pa_mutually_exclusive_i.clear();
        mutually_exclusive_headers.clear();
        return rv;
    }

    /** Get global pragma pa_mutually_exclusive.
     */
    bool preorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaMutuallyExclusive(const PhvInfo &phv) : phv_i(phv) {}

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    const ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> &mutex_fields() const {
        return pa_mutually_exclusive_i;
    }
    const ordered_map<cstring, ordered_set<cstring>> &mutex_headers() const {
        return mutually_exclusive_headers;
    }
};

std::ostream &operator<<(std::ostream &out, const PragmaMutuallyExclusive &pa_me);

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_MUTUALLY_EXCLUSIVE_H_ */
