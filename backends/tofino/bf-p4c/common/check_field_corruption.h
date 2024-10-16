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

#ifndef BF_P4C_COMMON_CHECK_FIELD_CORRUPTION_H_
#define BF_P4C_COMMON_CHECK_FIELD_CORRUPTION_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "ir/ir.h"

using namespace P4;

/**
 * @ingroup parde
 */
class CheckFieldCorruption : public Inspector, TofinoWriteContext {
 private:
    const FieldDefUse &defuse;
    const PhvInfo &phv;
    const PHV::Pragmas &pragmas;
    Phv_Parde_Mau_Use uses;
    ordered_set<const PHV::Field*> pov_protected_fields;
    std::map<const IR::BFN::ParserState *, std::set<const IR::Expression *>> state_extracts;
    std::map<const PHV::Field *, std::set<const IR::Expression *>> parser_inits;

 protected:
    /// Check if any other fields that share a container with the field in @p use are extracted
    /// from packet data independenty from this field and are not mutually exclusive.
    ///
    /// @param use Use to check if there are other parser extractions
    ///
    /// @return Boolean value indicating whether one or more other fields share a container with
    ///         @p f that are extracted from packet data and not mutually exclusive.
    bool copackedFieldExtractedSeparately(const FieldDefUse::locpair& use);

 public:
    CheckFieldCorruption(
        const FieldDefUse &defuse,
        const PhvInfo &phv,
        const PHV::Pragmas &pragmas) :
        defuse(defuse), phv(phv), pragmas(pragmas), uses(phv) {}

    void end_apply() override;
    bool preorder(const IR::BFN::Pipe *) override;
    bool preorder(const IR::BFN::DeparserParameter *) override;
    bool preorder(const IR::BFN::Digest *) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::BFN::ParserZeroInit *) override;
};

#endif /* BF_P4C_COMMON_CHECK_FIELD_CORRUPTION_H_ */
