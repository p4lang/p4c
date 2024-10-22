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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_ALIAS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_ALIAS_H_

#include <map>
#include <optional>

#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_no_overlay.h"
#include "bf-p4c/phv/pragma/pretty_print.h"
#include "bf-p4c/phv/utils/utils.h"
#include "ir/ir.h"

using namespace P4;
/** pa_alias pragma support.
 *
 * This pass will gather all pa_alias pragmas and generate pa_alias_i:
 * mapping from a field to another field that it aliases with.
 *
 */
class PragmaAlias : public Inspector, public Pragma::PrettyPrint {
 public:
    enum CreatedBy { PRAGMA, COMPILER };

    struct AliasDestination {
        /// The alias destination field.
        cstring field;

        /// The range of the field being aliased, or std::nullopt when the
        /// entire field is aliased.
        std::optional<le_bitrange> range;

        /// Who created the pragma?
        CreatedBy who;
    };

    /// Map type from alias sources to destinations.
    using AliasMap = ordered_map<cstring, AliasDestination>;

 private:
    const PhvInfo &phv_i;
    AliasMap aliasMap;
    PragmaNoOverlay &no_overlay;

    /// All PHV::Field objects that have expressions associated with them.
    /// This is used to replace IR::Expression objects for aliased fields later.
    bitvec fieldsWithExpressions;
    /// All fields involved in aliasing operations as a source
    bitvec fieldsWithAliasingSrc;
    /// All fields involved in aliasing operations as a destination
    bitvec fieldsWithAliasingDst;

    profile_t init_apply(const IR::Node *root) override;

    /// Get all fields with IR::Expression objects associated with them.
    bool preorder(const IR::Expression *expr) override;

    /// Get global pragma pa_alias.
    void postorder(const IR::BFN::Pipe *pipe) override;

 public:
    explicit PragmaAlias(PhvInfo &phv, PragmaNoOverlay &no_ovrl)
        : phv_i(phv), no_overlay(no_ovrl) {}
    const AliasMap &getAliasMap() const { return aliasMap; }

    /// BFN::Pragma interface
    static const char *name;
    static const char *description;
    static const char *help;

    /// Checks if alias is possible between @p f1 and @p f2. If not, returns std::nullopt.
    /// If yes, it returns a pair of fields, the first being the alias destination and the
    /// second is the alias source (field being replaced).
    std::optional<std::pair<const PHV::Field *, const PHV::Field *>> mayAddAlias(
        const PHV::Field *f1, const PHV::Field *f2, bool suppressWarning = false,
        CreatedBy who = PRAGMA);

    bool addAlias(const PHV::Field *f1, const PHV::Field *f2, bool suppressWarning = false,
                  CreatedBy who = PRAGMA);

    std::string pretty_print() const override;
};

std::ostream &operator<<(std::ostream &out, const PragmaAlias &pa_a);
std::ostream &operator<<(std::ostream &, const PragmaAlias::AliasDestination &dest);

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_PRAGMA_PA_ALIAS_H_ */
