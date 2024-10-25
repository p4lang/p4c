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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_BUILD_MUTEX_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_BUILD_MUTEX_H_

#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/bitvec.h"
#include "lib/cstring.h"
#include "lib/symbitmatrix.h"

/* Produces a SymBitMatrix where keys are PHV::Field ids and values indicate
 * whether two fields are mutually exclusive, based on analyzing the structure
 * of the control flow graph to identify fields that are not live at the same
 * time.
 *
 * ALGORITHM: Build @mutually_inclusive, which denotes pairs of fields that are
 * live on the same control-flow path.  All fields in @fields_encountered that
 * are not mutually inclusive are mutually exclusive.
 *
 * For example, BuildParserOverlay inherits from BuildMutex, traversing the
 * parser control flow graph (and nothing else), and ignoring non-header
 * fields, to produce header fields that cannot be parsed from the same packet.
 *
 * @warning Take care when traversing a subset of the IR, because this might
 * produce fields that are mutually exclusive in that subgraph but not
 * throughout the entire IR.  For example, header fields that are mutually
 * exclusive in the parser may be added to the same packet in the MAU pipeline.
 *
 * Takes as an argument a set of fields that can be added in the MAU pipeline.
 * These fields are never considered to be mutually exclusive with any other
 * field based on this analysis of the parser.
 *
 * For example, many P4 parsers accept packets that have either an IPv4 or IPv6
 * header, but not both.  These headers are considered mutually exclusive.
 * However, suppose an `add_header(ipv4)` instruction exists in the MAU
 * pipeline, and fields in the IPv4 header are supplied to this pass.  In that
 * case, IPv4 and IPv6 header fields are not considered mutually exclusive.
 *
 * This class is intended to be specialized in two ways: to tailor which parts
 * of the pipeline are visited, and to tailor which kinds of fields are
 * considered.  @see BuildParserOverlay, BuildMetadataOverlay.
 */
class BuildMutex : public BFN::ControlFlowVisitor, public Inspector {
 public:
    using FieldFilter_t = std::function<bool(const PHV::Field *f)>;

 protected:
    PhvInfo &phv;
    const bitvec &neverOverlay;
    const PragmaNoOverlay &pragma;

    /// If mutually_inclusive(f1->id, f2->id), then fields f1 and f2 are used
    /// or defined on the same control flow path.
    SymBitMatrix mutually_inclusive;

    /// If mutually_inclusive(f1, f2) == false, i.e. f1 and f2 never appear on
    /// the same control flow path, then f1 and f2 are mutually exclusive.
    SymBitMatrix &mutually_exclusive;

    /// @returns true if @p f should be ignored in this analysis.
    FieldFilter_t IgnoreField;

    /// Tracks the fields encountered (and not ignored) during this analysis.
    bitvec fields_encountered;

    virtual void mark(const PHV::Field *);

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::Expression *) override;
    bool preorder(const IR::MAU::Action *act) override;
    void flow_merge(Visitor &) override;
    void flow_copy(::ControlFlowVisitor &) override;
    void end_apply() override;

 public:
    BuildMutex(PhvInfo &phv, const bitvec &neverOverlay, const PragmaNoOverlay &pragma,
               FieldFilter_t ignore_field)
        : phv(phv),
          neverOverlay(neverOverlay),
          pragma(pragma),
          mutually_exclusive(phv.field_mutex()),
          IgnoreField(ignore_field) {
        joinFlows = true;
        visitDagOnce = false;
        BackwardsCompatibleBroken = true;
    }

    BuildMutex *clone() const override { return new BuildMutex(*this); }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_BUILD_MUTEX_H_  */
