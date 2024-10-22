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

#ifndef BF_P4C_PHV_AUTO_INIT_METADATA_H_
#define BF_P4C_PHV_AUTO_INIT_METADATA_H_

#include <set>
#include <utility>

#include "bf-p4c/common/elim_unused.h"
#include "bf-p4c/common/field_defuse.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/cstring.h"

/// By default, the compiler automatically initializes metadata at the start of each gress.
/// This pass disables this automatic initialization by adding \@pa_no_init annotations to all
/// metadata fields that might not be explicitly written by the time they are read.
class DisableAutoInitMetadata : public Transform {
    const FieldDefUse &defuse;
    const PhvInfo &phv;

 public:
    explicit DisableAutoInitMetadata(const FieldDefUse &defuse, const PhvInfo &phv)
        : defuse(defuse), phv(phv) {}

    const IR::Node *preorder(IR::BFN::Pipe *pipe) override;

 private:
    /// @return true iff the user requested automatic initialization of metadata.
    bool auto_init_metadata(const IR::BFN::Pipe *pipe) const;
};

/// Removes unnecessary metadata initializations. An assignment to a metadata field is deemed an
/// unnecessary initialization if it satisfies all of the following:
///
///   - The metadata field is not annotated with \@pa_no_init.
///   - The metadata field is initially valid (@see PHV::Field::is_invalidate_from_arch).
///   - The constant 0 is assigned.
///   - The assignment only overwrites ImplicitParserInit, according to def-use.
class RemoveMetadataInits : public AbstractElimUnusedInstructions {
    const PhvInfo &phv;
    const FieldDefUse &defuse;
    std::set<cstring> &zeroInitFields;

    /// The set of fields with a \@pa_no_init annotation. Each field is represented by
    /// \a gress::hdr.field.
    std::set<cstring> pa_no_inits;

    /// Determines whether an assignment from the given right expression to the given left
    /// expression in the given unit should be eliminated.
    bool elim_assign(const IR::BFN::Unit *unit, const IR::Expression *left,
                     const IR::Expression *right);

    profile_t init_apply(const IR::Node *root) override;
    void end_apply() override;

 public:
    bool elim_extract(const IR::BFN::Unit *unit, const IR::BFN::Extract *extract) override;

    const IR::BFN::Pipe *preorder(IR::BFN::Pipe *pipe) override;
    const IR::MAU::Instruction *preorder(IR::MAU::Instruction *instr) override;

    explicit RemoveMetadataInits(const PhvInfo &phv, const FieldDefUse &defuse,
                                 std::set<cstring> &zeroInitFields)
        : AbstractElimUnusedInstructions(defuse),
          phv(phv),
          defuse(defuse),
          zeroInitFields(zeroInitFields) {}
};

#endif /* BF_P4C_PHV_AUTO_INIT_METADATA_H_ */
