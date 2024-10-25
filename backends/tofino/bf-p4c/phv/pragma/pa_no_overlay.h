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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_OVERLAY_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_OVERLAY_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

using namespace P4;

/** Adds the no_overlay pragma to the specified fields with the specified gress.
 * (1) When only one field is provided in a pa_no_overlay pragma,
 *     fields with this pragma cannot be overlaid with any other field.
 * (2) When more than one field is provided, fields in this group cannot
 *     be overlaid with each other, i.e., fields are mutually inclusive.
 */
class PragmaNoOverlay : public Inspector {
    PhvInfo &phv_i;

    /// List of fields for which the pragma pa_no_overlay has been specified that
    /// cannot be overlaid with all other fields.
    ordered_set<const PHV::Field *> no_overlay;
    /// mutually_inclusive(field1->id, field2->id) = true means that field1 and field2
    /// cannot be overlaid with each other.
    SymBitMatrix mutually_inclusive;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        no_overlay.clear();
        mutually_inclusive.clear();
        return rv;
    }

    bool preorder(const IR::BFN::Pipe *pipe) override;
    bool preorder(const IR::MAU::Instruction *inst) override;

 public:
    explicit PragmaNoOverlay(PhvInfo &phv) : phv_i(phv) {}

    friend std::ostream &operator<<(std::ostream &out, const PragmaNoOverlay &pa_no);
    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    /// @returns the set of fields fo which the pragma pa_no_overlay has been specified in the
    /// program.
    const ordered_set<const PHV::Field *> &get_no_overlay_fields() const { return no_overlay; }

    bool can_overlay(const PHV::Field *a, const PHV::Field *b) const {
        return !no_overlay.count(a) && !no_overlay.count(b) && !mutually_inclusive(a->id, b->id);
    }
};

std::ostream &operator<<(std::ostream &out, const PragmaNoOverlay &pa_no);

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_NO_OVERLAY_H_ */
