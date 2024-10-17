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

#ifndef BF_P4C_PHV_ANALYSIS_META_LIVE_RANGE_H_
#define BF_P4C_PHV_ANALYSIS_META_LIVE_RANGE_H_

#include "ir/ir.h"
#include "lib/log.h"
#include "lib/symbitmatrix.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/mau_backtracker.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"

/** This class calculates the live range of metadata fields (as well as other fields whose live
  * ranges are affected by programmer specified pragmas). The calculated live ranges use the
  * min_stage value for tables determined using the table dependency graph.
  *
  * This pass will be a producer for a future LiveRangeShrinking class which will use these
  * calculated live ranges to speculatively shrink the live ranges for metadata fields and
  * initialize those fields accordingly, thereby creating considerably more opportunities for
  * metadata fields sharing PHVs.
  */
class MetadataLiveRange : public Inspector {
 public:
    // TODO: Should we expose a way of changing the DEP_DIST using a command line flag, as
    // glass does?
    static constexpr int DEP_DIST = 2;
    /// Name of the ingress parser state, where the compiler adds all the implicit initializations
    /// for fields with uninitialized reads.
    static constexpr char const *INGRESS_PARSER_ENTRY =
        "$entry_point.$ingress_tna_entry_point";
    /// Name of the egress parser state, where the compiler adds all the implicit initializations
    /// for fields with uninitialized reads.
    static constexpr char const *EGRESS_PARSER_ENTRY =
        "$entry_point.$egress_tna_entry_point";

    /// @returns true if the live ranges indicated by [@p minStage1, @p maxStage1]
    /// and [@p minStage2, @p maxStage2] are not-overlapping with clearance of at least @p depDist
    /// stages.
    static bool notOverlapping(int minStage1, int maxStage1, int minStage2, int maxStage2,
                               int depDist = DEP_DIST);

    /// @returns true if the live ranges indicated by @p range1 @p range2 are not-overlapping with
    /// clearance of at least @p depDist stages.
    static bool notOverlapping(std::pair<int, int>& range1, std::pair<int, int>& range2,
                               int depDist = DEP_DIST);

    /// Set of intrinsic metadata fields that must not be initialized.
    static const ordered_set<cstring> noInitIntrinsicFields;

    /// Type of access in a unit. OR these together for a Read/Write access (RW=3).
    enum access_t { READ = 1, WRITE = 2 };

    /// @returns a string representation of an access value.
    static cstring printAccess(int access);

 private:
    PhvInfo                                 &phv;
    const DependencyGraph                   &dg;
    FieldDefUse                             &defuse;
    const PragmaNoOverlay                   &noOverlay;
    const PhvUse                            &uses;

    /// List of fields that are marked as pa_no_init, which means that we assume the live range of
    /// these fields is from the first use of it to the last use.
    const ordered_set<const PHV::Field*>    &noInitFields;
    /// List of fields that are marked as not parsed.
    const ordered_set<const PHV::Field*>    &notParsedFields;
    /// List of fields that are marked as not deparsed.
    const ordered_set<const PHV::Field*>    &notDeparsedFields;

    /// overlay(f1->id, f2->id) true if the live ranges of fields f1 and f2 allow overlay. Overlay
    /// is considered possible if the live ranges according to the table dependency graph's
    /// `min_stage` calculation have a stage separation of DEP_DIST or more.
    SymBitMatrix&                           overlay;

    /// Map of field ID to the live range for each field.
    ordered_map<int, std::pair<int, int>>  livemap;
    /// Map of field ID to the access type (read, write, or read/write) in the min and max stage.
    ordered_map<int, std::pair<int, int>>  livemapUsage;

    /// Map of stage to the set of tables whose min_stage is the key stage.
    ordered_map<int, ordered_set<const IR::MAU::Table*>> minStages;

    profile_t init_apply(const IR::Node* root) override;
    bool preorder(const IR::MAU::Table* t) override;
    void end_apply() override;

    /// Pretty print the live ranges of all metadata fields.
    void printLiveRanges() const;

    /// Calculate and set the live range for field @p f.
    void setFieldLiveMap(const PHV::Field* f);

    /// Padding fields have special requirements for live ranges: they are only alive at the ingress
    /// deparser and the egress parser. This function sets the live range for a padding field @p f.
    void setPaddingFieldLiveMap(const PHV::Field* f);

 public:
    /// @returns the live ranges of all metadata fields: key is field ID and the value is the live
    /// range pair [minStage, maxStage], where parser = -1 and deparser = Device::numStages().
    const ordered_map<int, std::pair<int, int>>& getMetadataLiveMap() const {
        return livemap;
    }

    /// @returns the usages of the min and max stage for all metadata fields: key is field ID and
    /// the value is the access_t pair [access in minStage, access in maxStage].
    const ordered_map<int, std::pair<int, int>>& getMetadataLiveMapUsage() const {
        return livemapUsage;
    }

    /// @returns a map of stage to the set of tables whose min_stage is the key stage.
    const ordered_map<int, ordered_set<const IR::MAU::Table*>>& getMinStageMap() const {
        return minStages;
    }

    /// @returns the set of tables that have min_stage equal to @p stage.
    const ordered_set<const IR::MAU::Table*> getTablesInStage(int stage) const {
        if (!minStages.count(stage))
            BUG("Stage %1% not in the resource-insensitive tabel dependence graph", stage);
        return minStages.at(stage);
    }

    /// @returns true if fields @p f1 and @p f2 are found to be potentially overlayable
    /// because of their live ranges.
    bool hasPotentialLiveRangeOverlay(const PHV::Field* f1, const PHV::Field* f2) const {
        return overlay(f1->id, f2->id);
    }

    explicit MetadataLiveRange(
            PhvInfo& p,
            const DependencyGraph& g,
            FieldDefUse& f,
            const PHV::Pragmas& pragmas,
            const PhvUse& u,
            const MauBacktracker&)
        : phv(p),
          dg(g),
          defuse(f),
          noOverlay(pragmas.pa_no_overlay()),
          uses(u),
          noInitFields(pragmas.pa_no_init().getFields()),
          notParsedFields(pragmas.pa_deparser_zero().getNotParsedFields()),
          notDeparsedFields(pragmas.pa_deparser_zero().getNotDeparsedFields()),
          overlay(phv.metadata_mutex())
    { }
};

#endif  /* BF_P4C_PHV_ANALYSIS_META_LIVE_RANGE_H_ */
