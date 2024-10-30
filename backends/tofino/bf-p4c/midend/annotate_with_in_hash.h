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

/**
 * \defgroup AnnotateWithInHash BFN::AnnotateWithInHash
 * \ingroup midend
 * \brief Set of passes that annotate specific assignment statements with the \@in_hash annotation.
 *
 * The purpose is to eliminate some %PHV and PARDE alignment issues caused by
 * statements mapped to ALU units, namely binary arithmetic operations whose
 * operands are casted or concatenated.
 *
 * E.g.:
 *
 *     meta.index = meta.index + (3w0 ++ hdr.vlan.pcp);
 *
 * is converted to:
 *
 *     \@in_hash { meta.index = meta.index + (3w0 ++ hdr.vlan.pcp); }
 *
 * The first statement looked originally like:
 *
 *     meta.index = meta.index + (bit<6>)hdr.vlan.pcp;
 *
 * And was modified with the ElimCasts pass.
 *
 * The pass performs several checks to be sure that annotating is added
 * only if it can help with dealing an issue. Namely, it checks whether
 * the operands of the arithmetic operation do not have appropriate width
 * for ALU units (i.e. 8, 16, or 32 bits).
 *
 * @pre The pass has to be preceeded with the BFN::ElimCasts pass, which replaces
 * the cast operation with zero-extension using the concatenation operator, and
 * with the P4::SynthesizeActions pass, which moves assignment statements from
 * apply blocks into actions where annotating with the \@in_hash annotation happens.
 *
 * Also see the limitations of the \@in_hash annotation.
 */
#ifndef BACKENDS_TOFINO_BF_P4C_MIDEND_ANNOTATE_WITH_IN_HASH_H_
#define BACKENDS_TOFINO_BF_P4C_MIDEND_ANNOTATE_WITH_IN_HASH_H_

#include "bf-p4c/midend/type_checker.h"
#include "ir/ir.h"

namespace BFN {

/**
 * \ingroup AnnotateWithInHash
 */
class DoAnnotateWithInHash : public Transform {
    P4::TypeMap *typeMap;

    bool checkKeyDefaultAction(const IR::P4Control &control, const IR::P4Action &action) const;
    bool checkAssignmentStructure(const IR::AssignmentStatement &assignment,
                                  const IR::Expression **op, const IR::Expression **opConcat);
    bool checkAluSuitability(const IR::Expression &op) const;
    bool checkHeaderMetadataReference(const IR::Expression &op) const;

 public:
    explicit DoAnnotateWithInHash(P4::TypeMap *typeMap) : typeMap(typeMap) {}
    const IR::Node *preorder(IR::BlockStatement *b) override;
    const IR::Node *preorder(IR::Expression *e) override;
    const IR::Node *preorder(IR::AssignmentStatement *assignment) override;
};

/**
 * \ingroup AnnotateWithInHash
 * \brief Top level PassManager that governs annotation of specific assignment
 *        statements with the \@in_hash annotation.
 */

class AnnotateWithInHash : public PassManager {
 public:
    AnnotateWithInHash(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                       BFN::TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new BFN::TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoAnnotateWithInHash(typeMap));
    }
};

}  // namespace BFN

#endif  // BACKENDS_TOFINO_BF_P4C_MIDEND_ANNOTATE_WITH_IN_HASH_H_
