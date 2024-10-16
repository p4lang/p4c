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

#ifndef EXTENSIONS_BF_P4C_COMMON_ALIAS_H_
#define EXTENSIONS_BF_P4C_COMMON_ALIAS_H_

#include "ir/ir.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_alias.h"
#include "bf-p4c/phv/transforms/auto_alias.h"

using namespace P4;

/** This class generates a fieldExpressions map that stores a representative IR::Expression object
  * corresponding to each field in the program.
  */
class FindExpressionsForFields : public Inspector {
 private:
    /// Map of IR::Expression objects corresponding to the field names.
    ordered_map<cstring, const IR::Member*>& fieldNameToExpressionsMap;

    profile_t init_apply(const IR::Node* root) override;
    bool preorder(const IR::HeaderOrMetadata* h) override;

 public:
    FindExpressionsForFields(const PhvInfo&, ordered_map<cstring, const IR::Member*>& f)
        : fieldNameToExpressionsMap(f) { }
};

/** This class replaces all uses of alias source fields with the corresponding alias destination
  * fields. It uses the fieldExpressions map populated by the FindExpressionsForFields pass to
  * generate new expressions.
  */
class ReplaceAllAliases : public Transform {
 private:
    /// PhvInfo object.
    const PhvInfo& phv;
    /// Aliasing information from the \@pa_alias pragma.
    const PragmaAlias& pragmaAlias;
    /// Map of IR::Expression objects corresponding to the alias destination fields.
    const ordered_map<cstring, const IR::Member*>&    fieldExpressions;

    profile_t init_apply(const IR::Node* root) override;
    IR::Node* preorder(IR::BFN::Pipe*) override;
    IR::Node* preorder(IR::Expression* expr) override;

 public:
    ReplaceAllAliases(
            const PhvInfo& p,
            const PragmaAlias& pragmaAlias,
            const ordered_map<cstring, const IR::Member*>& f)
        : phv(p), pragmaAlias(pragmaAlias), fieldExpressions(f) { }
};

/** For alias sources with explicit validity bits that are replaced during the aliasing transform,
  * we need to set the corresponding validity bit to true, in every action where the alias
  * destination is now written. This pass effects that transform.
  */
class AddValidityBitSets : public Transform {
 private:
    const PhvInfo&                  phv;
    const PragmaAlias&              pragmaAlias;

    ordered_map<const PHV::Field*, ordered_set<const PHV::Field*>> aliasedFields;
    ordered_map<cstring, ordered_set<const PHV::Field*>> actionToPOVMap;

    inline cstring getFieldValidityName(cstring origName);

    profile_t init_apply(const IR::Node* root) override;
    IR::Node* preorder(IR::MAU::Instruction* inst) override;
    IR::Node* postorder(IR::MAU::Action* action) override;

 public:
    explicit AddValidityBitSets(const PhvInfo& p, const PragmaAlias& pa)
        : phv(p), pragmaAlias(pa) { }
};

/** This class implements a pass manager that handles aliasing relationships specified by pa_alias
  * fields. The general idea is that all the uses of one of the fields specified by the pragma
  * (alias source) are replaced by the uses of the other field in the pragma (alias destination). In
  * cases where one of the fields is a header and the other is metadata, the metadata is always the
  * alias source and the header field is always the alias destination. When both fields are
  * metadata, PragmaAlias pass chooses one of them as the alias source. Aliasing is not allowed when
  * both fields belong to packet headers.
  */
class Alias : public Logging::PassManager {
 private:
     ordered_map<cstring, const IR::Member*> fieldExpressions;
     const PragmaAlias &pragmaAlias;

 public:
    explicit Alias(PhvInfo& phv, const PragmaAlias &pa) :
        Logging::PassManager("pragmas"_cs, Logging::Mode::AUTO), pragmaAlias(pa) {
        addPasses({
            new FindExpressionsForFields(phv, fieldExpressions),
            new AddValidityBitSets(phv, pragmaAlias),
            new ReplaceAllAliases(phv, pragmaAlias, fieldExpressions),
            new VisitFunctor([this]() {
                if (LOGGING(1)) {
                    LOG1(pragmaAlias.pretty_print()); }
            })
        });
    }
};

/// Replace AliasMember and AliasSlice nodes with their alias sources.
class ReinstateAliasSources : public Transform {
    PhvInfo&    phv;

    IR::Node* preorder(IR::BFN::AliasMember* alias) override {
        const PHV::Field* aliasSource = phv.field(alias->source);
        const PHV::Field* aliasDestination = phv.field(alias);
        BUG_CHECK(aliasSource, "Field %1% not found", alias->source);
        BUG_CHECK(aliasDestination, "Field %1% not found", alias);
        phv.addAliasMapEntry(aliasSource, aliasDestination);
        return alias->source->apply(ReinstateAliasSources(phv))->clone();
    }

    IR::Node* preorder(IR::BFN::AliasSlice* alias) override {
        const PHV::Field* aliasSource = phv.field(alias->source);
        const PHV::Field* aliasDestination = phv.field(alias);
        BUG_CHECK(aliasSource, "Field %1% not found", alias->source);
        BUG_CHECK(aliasDestination, "Field %1% not found", alias);
        phv.addAliasMapEntry(aliasSource, aliasDestination);
        return alias->source->apply(ReinstateAliasSources(phv))->clone();
    }

 public:
    explicit ReinstateAliasSources(PhvInfo& p) : phv(p) { }
};

#endif  /* EXTENSIONS_BF_P4C_COMMON_ALIAS_H_ */
