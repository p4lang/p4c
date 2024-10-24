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

#ifndef BF_P4C_PHV_PHV_PARDE_MAU_USE_H_
#define BF_P4C_PHV_PHV_PARDE_MAU_USE_H_

#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/ir/thread_visitor.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/phv/phv.h"
#include "ir/ir.h"

using namespace P4;

namespace PHV {
class Field;
}  // namespace PHV

class PhvInfo;

class Phv_Parde_Mau_Use : public Inspector, public TofinoWriteContext {
    using FieldToRangeMap = ordered_map<const PHV::Field *, ordered_set<le_bitrange>>;
    using FieldIdToRangeMap = ordered_map<int, ordered_set<le_bitrange>>;

    /// Fields written in the MAU pipeline. Keys are fields written in the MAU pipeline. Values are
    /// all the different slices of the fields that are written.
    FieldIdToRangeMap written_i;

    /// Fields used in at least one ALU instruction. Keys are fields used in an ALU instruction.
    /// Values are the slices of the fields so used.
    FieldIdToRangeMap used_alu_i;

    /// field to ranges that was used as table key.
    ordered_map<
        const PHV::Field *,
        ordered_map<le_bitrange,
                    ordered_set<std::pair<const IR::MAU::Table *, const IR::MAU::TableKey *>>>>
        ixbar_read_i;

    /// fields that are read by deparser learning engine, including selector.
    ordered_set<const PHV::Field *> learning_reads_i;

    /// Fields extracted in the parser.
    bitvec extracted_i[GRESS_T_COUNT];

    /// Fields extracted in the parser from the packet stream
    bitvec extracted_from_pkt_i[GRESS_T_COUNT];

    /// Fields extracted in the parser from a constant.
    bitvec extracted_from_const_i[GRESS_T_COUNT];

    /// Handy enum for indexing use_i below.
    enum { PARDE = 0, MAU = 1 };

 public:
    /// Represents uses of fields in MAU/PARDE. Keys of the outer maps are all the field ids used in
    /// the program. The inner map represents how those fields are used. The key of the inner map
    /// represents where the use (read or write) happens in the PARDE (value 0) or MAU (value 1).
    /// The values in the inner map are the slices of the fields used in the respective units.
    ordered_map<int, ordered_map<unsigned, ordered_set<le_bitrange>>> use_i;

    /// Fields used in the deparser.
    bitvec deparser_i[GRESS_T_COUNT];
    /*                |    ^- gress               */
    /*                 == use in deparser         */

    explicit Phv_Parde_Mau_Use(const PhvInfo &p) : phv(p) {}

    /// @returns true if @p f is read or written anywhere in the pipeline
    /// or if @p f is marked bridged.
    bool is_referenced(const PHV::Field *f) const;

    /// @returns true if @p f is used in the deparser.
    bool is_deparsed(const PHV::Field *f) const;

    /// @returns true if @p f is used (read or written) in the MAU pipeline.
    bool is_used_mau(const PHV::Field *f) const;
    bool is_used_mau(const PHV::Field *f, le_bitrange range) const;

    /// @returns true if @p f is written in the MAU pipeline.
    bool is_written_mau(const PHV::Field *f) const;

    /// @returns true if @p f is used in an ALU instruction.
    bool is_used_alu(const PHV::Field *f) const;

    /// @returns true if @p f is used in the parser or deparser.
    bool is_used_parde(const PHV::Field *f) const;

    /// @returns true if @p f is extracted in the (ingress or egress) parser.
    bool is_extracted(const PHV::Field *f, std::optional<gress_t> gress = std::nullopt) const;

    /// @returns true if @p f is extracted in the (ingress or egress) parser from packet data.
    bool is_extracted_from_pkt(const PHV::Field *f,
                               std::optional<gress_t> gress = std::nullopt) const;

    /// @returns true if @p f is extracted in the (ingress or egress) parser from a constant.
    bool is_extracted_from_constant(const PHV::Field *f,
                                    std::optional<gress_t> gress = std::nullopt) const;

    /// @returns true if @p f needs to be allocated for PHV.
    /// A field must be allocated to a PHV if is_ignore_alloc() is false and
    /// (1) is referenced or
    /// (2) not referenced, but is ghost field (we should try to
    ///     eliminated this case once we add ghost field writes to IR).
    bool is_allocation_required(const PHV::Field *f) const;

    /// @returns true if @p is read by deparser for learning digest and subject to the
    /// maxDigestSizeInBytes constraint.
    bool is_learning(const PHV::Field *f) const { return learning_reads_i.count(f); };

    /// @returns ixbar read of sub-ranges to tables of @p range of @p f.
    /// premise: @p range must be a fine-sliced range that either all of or none of
    /// the bits in the range are read by ixbar. Otherwise, BUG will be thrown.
    ordered_set<std::pair<const IR::MAU::Table *, const IR::MAU::TableKey *>> ixbar_read(
        const PHV::Field *f, le_bitrange range) const;

 protected:
    const PhvInfo &phv;
    gress_t thread = INGRESS;
    bool in_mau = false;
    bool in_dep = false;

 protected:
    profile_t init_apply(const IR::Node *) override;
    bool preorder(const IR::BFN::Parser *) override;
    bool preorder(const IR::BFN::Extract *) override;
    bool preorder(const IR::BFN::ParserChecksumWritePrimitive *) override;
    bool preorder(const IR::BFN::Deparser *) override;
    bool preorder(const IR::BFN::Digest *) override;
    bool preorder(const IR::MAU::TableSeq *) override;
    bool preorder(const IR::MAU::TableKey *) override;
    bool preorder(const IR::Expression *) override;
};

/// Consider additional cases specific to Phv Allocation, e.g., treat
/// egress_port and digests as used in MAU as they can't go in TPHV.
class PhvUse : public Phv_Parde_Mau_Use {
 public:
    explicit PhvUse(const PhvInfo &p) : Phv_Parde_Mau_Use(p) {}

 private:
    bool preorder(const IR::BFN::Deparser *d) override;
    bool preorder(const IR::BFN::DeparserParameter *param) override;
    void postorder(const IR::BFN::DeparserParameter *param) override;
    bool preorder(const IR::BFN::Digest *digest) override;
    void postorder(const IR::BFN::Digest *digest) override;
};

#endif /* BF_P4C_PHV_PHV_PARDE_MAU_USE_H_ */
