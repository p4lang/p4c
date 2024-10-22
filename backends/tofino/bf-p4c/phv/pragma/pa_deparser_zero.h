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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_DEPARSER_ZERO_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_DEPARSER_ZERO_H_

#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir.h"

using namespace P4;

/** Adds the not_parsed and not_deparsed pragmas to the fields in the attached header and gress. The
 * fields marked in this way form the basis of the deparser zero optimization.
 */
class PragmaDeparserZero : public Inspector {
 private:
    PhvInfo &phv_i;

    /// Vector of all PHV pragmas handled by this class.
    static const std::vector<std::string> *supported_pragmas;

    /// List of fields for which the pragma not_parsed has been specified.
    ordered_set<const PHV::Field *> notParsedFields;
    /// List of fields for which the pragma not_deparsed has been specified.
    ordered_set<const PHV::Field *> notDeparsedFields;
    /// Map of header name to the list of fields in that header.
    ordered_map<cstring, ordered_set<const PHV::Field *>> headerFields;
    /// List of fields for which the deparse zero optimization has to be disabled.
    ordered_set<const PHV::Field *> disableDeparseZeroFields;

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Pipe *pipe) override;
    void end_apply() override;

 public:
    explicit PragmaDeparserZero(PhvInfo &p) : phv_i(p) {}

    /// @returns the set of fields for whose headers the not_parsed pragma has been specified.
    const ordered_set<const PHV::Field *> &getNotParsedFields() const { return notParsedFields; }

    /// @returns the set of fields for whose headers the not_deparsed pragma has been specified.
    const ordered_set<const PHV::Field *> &getNotDeparsedFields() const {
        return notDeparsedFields;
    }
};

std::ostream &operator<<(std::ostream &out, const PragmaDeparserZero &pa_np);

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_DEPARSER_ZERO_H_ */
