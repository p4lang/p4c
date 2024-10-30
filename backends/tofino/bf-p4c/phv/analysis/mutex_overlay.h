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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_MUTEX_OVERLAY_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_MUTEX_OVERLAY_H_

#include <iostream>

#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "bf-p4c/parde/parser_info.h"
#include "bf-p4c/phv/analysis/build_mutex.h"
#include "bf-p4c/phv/analysis/header_mutex.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/pa_no_pack.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace PHV {
class Field;
}  // namespace PHV

class PhvInfo;

/* Produces a SymBitMatrix where keys are PHV::Field ids and values
 * indicate whether two fields are mutually exclusive, based on analyzing the
 * structure of the parse graph to identify fields that can never appear in the
 * same packet.  @see BuildMutex.
 */
class BuildParserOverlay : public BuildMutex {
    /// Ignore non-header fields.
    static bool ignore_field(const PHV::Field *f) { return !f || f->pov || f->metadata; }

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::MAU::TableSeq *) override { return false; }
    bool preorder(const IR::BFN::Deparser *) override { return false; }

 public:
    BuildParserOverlay(PhvInfo &phv, const bitvec &neverOverlay, const PragmaNoOverlay &pragma)
        : BuildMutex(phv, neverOverlay, pragma, ignore_field) {}
    BuildParserOverlay *clone() const override { return new BuildParserOverlay(*this); }
};

class ExcludeParserLoopReachableFields : public Visitor {
    PhvInfo &phv;
    const MapFieldToParserStates &fieldToStates;
    const CollectParserInfo &parserInfo;

 private:
    const IR::Node *apply_visitor(const IR::Node *root, const char *) override;

    bool is_loop_reachable(const ordered_set<const IR::BFN::ParserState *> &k,
                           const ordered_set<const IR::BFN::ParserState *> &v);

 public:
    ExcludeParserLoopReachableFields(PhvInfo &phv, const MapFieldToParserStates &fs,
                                     const CollectParserInfo &pi)
        : phv(phv), fieldToStates(fs), parserInfo(pi) {}
};

/* Produces a SymBitMatrix where keys are PHV::Field ids and values indicate
 * whether two fields are mutually exclusive, based on analyzing the structure
 * of the MAU pipeline to identify metadata fields that are only used in
 * mutually exclusive tables/actions.  @see BuildMutex.
 */
class BuildMetadataOverlay : public BuildMutex {
 private:
    /// Ignore non-metadata fields.
    static bool ignore_field(const PHV::Field *f) { return !f || f->pov || !f->metadata; }

    profile_t init_apply(const IR::Node *root) override;
    bool preorder(const IR::BFN::Deparser *) override { return false; }

 public:
    BuildMetadataOverlay(PhvInfo &phv, const bitvec &neverOverlay, const PragmaNoOverlay &pragma)
        : BuildMutex(phv, neverOverlay, pragma, ignore_field) {}
    BuildMetadataOverlay *clone() const override { return new BuildMetadataOverlay(*this); }
};

/** Mark aliased header fields as never overlaid. When header fields are aliased, they are always
 * aliased with metadata, and should therefore be excluded from BuildParserOverlay.
 */
class ExcludeAliasedHeaderFields : public Inspector {
    PhvInfo &phv;
    bitvec &neverOverlay;

    bool preorder(const IR::BFN::AliasMember *alias) override {
        excludeAliasedField(alias);
        return true;
    }

    bool preorder(const IR::BFN::AliasSlice *alias) override {
        excludeAliasedField(alias);
        return true;
    }

    void excludeAliasedField(const IR::Expression *alias);

 public:
    ExcludeAliasedHeaderFields(PhvInfo &phv, bitvec &neverOverlay)
        : phv(phv), neverOverlay(neverOverlay) {}
};

/** Mark deparsed intrinsic metadata fields as never overlaid.  The deparser
 * reads the valid container bit for containers holding these fields, but
 * they're often never written.
 */
class ExcludeDeparsedIntrinsicMetadata : public Inspector {
    PhvInfo &phv;
    bitvec &neverOverlay;

    profile_t init_apply(const IR::Node *root) override {
        profile_t rv = Inspector::init_apply(root);
        neverOverlay.clear();
        return rv;
    }

    void end_apply() override;

 public:
    ExcludeDeparsedIntrinsicMetadata(PhvInfo &phv, bitvec &neverOverlay)
        : phv(phv), neverOverlay(neverOverlay) {}
};

/** Mark fields specified by pa_no_overlay as never overlaid. These fields are excluded from
 * BuildParserOverlay
 */
class ExcludePragmaNoOverlayFields : public Inspector {
    bitvec &neverOverlay;
    const PragmaNoOverlay &pragma;

    void end_apply() override;

 public:
    ExcludePragmaNoOverlayFields(bitvec &neverOverlay, const PragmaNoOverlay &p)
        : neverOverlay(neverOverlay), pragma(p) {}
};

/** Find fields that appear in headers that may be added (with the `add_header`
 * or `.setValid` instructions) in the MAU pipeline.  These fields are
 * (conservatively) excluded from BuildParserOverlay.
 *
 * @param rv holds field IDs of fields that cannot be overlaid.
 *
 * TODO: This analysis could be improved to determine whether an
 * `add_header` instruction is unreachable for certain classes of packets,
 * potentially making BuildParserOverlay more precise.
 *
 * @pre Must run after CopyHeaderEliminator transforms instances of
 * `add_header` and `setValid` to `modify_field`, and after
 * InstructionSelection transforms `modify_field` to `set`.
 */
class FindAddedHeaderFields : public MauInspector {
 private:
    PhvInfo &phv;
    bitvec rv;

    profile_t init_apply(const IR::Node *root) override {
        rv.clear();
        return Inspector::init_apply(root);
    }
    bool preorder(const IR::MAU::Primitive *prim) override;
    void markFields(const IR::HeaderRef *hr);

 public:
    explicit FindAddedHeaderFields(PhvInfo &phv) : phv(phv) {}
    bool isAddedInMAU(const PHV::Field *field) const { return rv[field->id]; }
};

class ExcludeMAUOverlays : public MauInspector {
 public:
    using ActionToFieldsMap = ordered_map<const IR::MAU::Action *, ordered_set<const PHV::Field *>>;

 private:
    PhvInfo &phv;
    const FindAddedHeaderFields &addedFields;

    ActionToFieldsMap actionToWrites;
    ActionToFieldsMap actionToReads;

    profile_t init_apply(const IR::Node *root) override {
        actionToWrites.clear();
        actionToReads.clear();
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::MAU::Table *tbl) override;
    bool preorder(const IR::MAU::Instruction *inst) override;
    void end_apply() override;

    /// Given map of action to fields @p arg, mark all fields corresponding to the same action as
    /// mutually non-exclusive.
    void markNonMutex(const ActionToFieldsMap &arg);

 public:
    explicit ExcludeMAUOverlays(PhvInfo &p, const FindAddedHeaderFields &a)
        : phv(p), addedFields(a) {}
};

/** Prevent overlaying of all fields that are emitted depending on the same POV bit.
 */
class ExcludeDeparserOverlays : public Inspector {
 private:
    PhvInfo &phv;
    ordered_map<const PHV::Field *, ordered_set<const PHV::Field *>> povToFieldsMap;

    profile_t init_apply(const IR::Node *root) override {
        povToFieldsMap.clear();
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::BFN::EmitField *emit) override;
    void end_apply() override;

 public:
    explicit ExcludeDeparserOverlays(PhvInfo &p) : phv(p) {}
};

/** Checksum is calculated only when the checksum destination field is valid.
 *  We need to make sure that the fields included in checksum don't overlap any
 *  other live fields when checksum destination is live. We do this by giving
 *  all the checksum fields same mutex constraint as that of checksum
 *  destination. Note: This contraint is added only for tofino because tofino's
 *  checksum engine cannot dynamically predicate entries based on header
 *  validity.  Instead, it relies on the PHV Validity bit.
 *
 *  Avoid overlay of checksum fields with other fields that may be
 *  present. The pass was updated to detect if checksum source field might come
 *  from a new header being added in MAU. In that case, all of the checksum
 *  source fields are set to be non mutually exclusive with every field being
 *  extracted but not referenced in MAU.
 */
class ExcludeCsumOverlays : public Inspector {
 private:
    PhvInfo &phv;
    const FindAddedHeaderFields &addedFields;
    const PhvUse &use;
    bool preorder(const IR::BFN::EmitChecksum *emitChecksum) override;

 public:
    explicit ExcludeCsumOverlays(PhvInfo &p, const FindAddedHeaderFields &a, const PhvUse &u)
        : phv(p), addedFields(a), use(u) {}
};

/** For checksums that rely on POV bits (Tofino2), make sure that none of
 *  the source fields are mutually exclusive with each other.
 *
 *  Note that source fields in POV-based checksums can be mutually exclusive
 *  with fields that are not used in checksums or that are used in a different
 *  checksum unit.
 */
class ExcludeCsumOverlaysPOV : public Inspector {
 private:
    PhvInfo &phv;
    const FindAddedHeaderFields &addedFields;
    const PhvUse &use;
    bool preorder(const IR::BFN::EmitChecksum *emitChecksum) override;

 public:
    explicit ExcludeCsumOverlaysPOV(PhvInfo &p, const FindAddedHeaderFields &a, const PhvUse &u)
        : phv(p), addedFields(a), use(u) {}
};

class MarkMutexPragmaFields : public Inspector {
 private:
    PhvInfo &phv;
    const PragmaMutuallyExclusive &pragma;

    profile_t init_apply(const IR::Node *root) override;

 public:
    explicit MarkMutexPragmaFields(PhvInfo &p, const PragmaMutuallyExclusive &pr)
        : phv(p), pragma(pr) {}
};

/// @see BuildParserOverlay and FindAddedHeaderFields.
class MutexOverlay : public PassManager {
 private:
    /// Field IDs of fields that cannot be overlaid.
    bitvec neverOverlay;
    FindAddedHeaderFields addedFields;
    CollectParserInfo parserInfo;
    MapFieldToParserStates fieldToParserStates;

 public:
    MutexOverlay(PhvInfo &phv, const PHV::Pragmas &pragmas, const PhvUse &use)
        : addedFields(phv), fieldToParserStates(phv) {
        Visitor *exclude_csum_overlays = nullptr;
        if (Device::currentDevice() == Device::TOFINO) {
            exclude_csum_overlays = new ExcludeCsumOverlays(phv, addedFields, use);
        } else if (Device::currentDevice() == Device::JBAY) {
            exclude_csum_overlays = new ExcludeCsumOverlaysPOV(phv, addedFields, use);
        }

        addPasses({new ExcludeDeparsedIntrinsicMetadata(phv, neverOverlay),
                   new ExcludePragmaNoOverlayFields(neverOverlay, pragmas.pa_no_overlay()),
                   &addedFields, new ExcludeAliasedHeaderFields(phv, neverOverlay),
                   new BuildParserOverlay(phv, neverOverlay, pragmas.pa_no_overlay()),
                   new BuildMetadataOverlay(phv, neverOverlay, pragmas.pa_no_overlay()),
                   &parserInfo, &fieldToParserStates,
                   new ExcludeParserLoopReachableFields(phv, fieldToParserStates, parserInfo),
                   exclude_csum_overlays, new ExcludeMAUOverlays(phv, addedFields),
                   new ExcludeDeparserOverlays(phv), new HeaderMutex(phv, neverOverlay, pragmas),
                   new MarkMutexPragmaFields(phv, pragmas.pa_mutually_exclusive())});
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_ANALYSIS_MUTEX_OVERLAY_H_  */
