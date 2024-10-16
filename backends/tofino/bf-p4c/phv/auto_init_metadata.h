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

#ifndef BF_P4C_PHV_AUTO_INIT_METADATA_H_
#define BF_P4C_PHV_AUTO_INIT_METADATA_H_

#include <set>
#include <utility>
#include "lib/cstring.h"
#include "ir/ir.h"
#include "ir/visitor.h"

#include "bf-p4c/common/elim_unused.h"
#include "bf-p4c/common/field_defuse.h"

/// By default, the compiler automatically initializes metadata at the start of each gress.
/// This pass disables this automatic initialization by adding \@pa_no_init annotations to all
/// metadata fields that might not be explicitly written by the time they are read.
class DisableAutoInitMetadata : public Transform {
    const FieldDefUse& defuse;
    const PhvInfo& phv;

 public:
    explicit DisableAutoInitMetadata(const FieldDefUse& defuse, const PhvInfo & phv) :
      defuse(defuse), phv(phv) { }

    const IR::Node* preorder(IR::BFN::Pipe* pipe) override;

 private:
    /// @return true iff the user requested automatic initialization of metadata.
    bool auto_init_metadata(const IR::BFN::Pipe* pipe) const;
};

/// Removes unnecessary metadata initializations. An assignment to a metadata field is deemed an
/// unnecessary initialization if it satisfies all of the following:
///
///   - The metadata field is not annotated with \@pa_no_init.
///   - The metadata field is initially valid (@see PHV::Field::is_invalidate_from_arch).
///   - The constant 0 is assigned.
///   - The assignment only overwrites ImplicitParserInit, according to def-use.
class RemoveMetadataInits : public AbstractElimUnusedInstructions {
    const PhvInfo& phv;
    const FieldDefUse& defuse;
    std::set<cstring> &zeroInitFields;

    /// The set of fields with a \@pa_no_init annotation. Each field is represented by
    /// \a gress::hdr.field.
    std::set<cstring> pa_no_inits;

    /// Determines whether an assignment from the given right expression to the given left
    /// expression in the given unit should be eliminated.
    bool elim_assign(const IR::BFN::Unit* unit,
                     const IR::Expression* left,
                     const IR::Expression* right);

    profile_t init_apply(const IR::Node* root) override;
    void end_apply() override;

 public:
    bool elim_extract(const IR::BFN::Unit* unit, const IR::BFN::Extract* extract) override;

    const IR::BFN::Pipe* preorder(IR::BFN::Pipe* pipe) override;
    const IR::MAU::Instruction* preorder(IR::MAU::Instruction* instr) override;

    explicit RemoveMetadataInits(const PhvInfo& phv, const FieldDefUse& defuse,
            std::set<cstring> &zeroInitFields)
      : AbstractElimUnusedInstructions(defuse), phv(phv), defuse(defuse),
        zeroInitFields(zeroInitFields) { }
};

#endif  /* BF_P4C_PHV_AUTO_INIT_METADATA_H_ */
