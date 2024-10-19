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
