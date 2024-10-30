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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_FIELDSLICE_LIVE_RANGE_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_FIELDSLICE_LIVE_RANGE_H_

#include <algorithm>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/common/map_tables_to_actions.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/parde/clot/clot_info.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/safe_vector.h"
#include "mau_backtracker.h"

namespace PHV {

class LiveRangeInfo {
 public:
    // OpInfo represents the `liveness requirement` of a fieldslice of a stage (including PARDE).
    // DEAD:       fieldslice does not need to be allocated.
    // READ:       fieldslice needs to be allocated before VLIW.
    // WRITE:      fieldslice needs to be allocated after VLIW.
    // READ_WRITE: fieldslice needs to be allocated before and after VLIW,
    //             (could have different containers for read and write)
    // LIVE:       fieldslice needs to be allocated before and after stage.
    //             (must be the same container)
    // Note that LIVE and READ_WRITE have almost the same `liveness requirement`, they are
    // different only when we compute disjoint live ranges, where READ_WRITE could be
    // a start of a new live range (changing container) but LIVE does not.
    // Consider two paired-defuse examples,
    // f1's paired defuse(s): [-1W, 0R], [0W, 2R] // will produce [-1W, 0RW, 1L, 2R]
    // f2's paired defuse(s): [-1, 2R]            // will produce [-1W, 0L, 1L, 2R]
    // f1 have two disjoint live ranges and they can be allocated to different containers
    // but f2 has to be allocated into one container.
    enum class OpInfo { DEAD, READ, WRITE, READ_WRITE, LIVE };

 private:
    /// Number of table stages
    /// Initialized to zero, but calls to set_num_table_stages enforce a minimum of
    /// `Device::numStages()`. (Can't enforce this minimum at initialization time as the device is
    /// not yet know.)
    static int num_table_stages;

    // -1 = parser
    // 0..num_table_stages - 1 = physical MAU stages
    // num_table_stages = deparser
    // we merge all together because we need to uniformly iterate
    // parser + mau + deparser.
    safe_vector<OpInfo> lives_i;

 public:
    LiveRangeInfo() {
        // Static variable can't be initialized to Device::numStages() as the device isn't known at
        // initialization time. Instead, check here that it's been set to a positive value.
        if (num_table_stages <= 0) num_table_stages = Device::numStages();
        lives_i.resize(num_table_stages + 2, OpInfo::DEAD);
    }
    OpInfo &parser() { return lives_i[0]; }
    const OpInfo &parser() const { return lives_i[0]; }
    OpInfo &deparser() { return lives_i[num_table_stages + 1]; }
    const OpInfo &deparser() const { return lives_i[num_table_stages + 1]; }
    OpInfo &stage(int i) { return lives_i[i + 1]; }
    const OpInfo &stage(int i) const { return lives_i[i + 1]; }

    /// Record the number of table stages in use
    ///
    /// Minimum size is `Device::numStages()`
    static void set_num_table_stages(int stage) {
        num_table_stages = std::max(stage, Device::numStages());
    }

    /// @returns live info in a vector: [Parser 0 1 ... max_stage Deparser]
    const safe_vector<OpInfo> &vec() const { return lives_i; }

    /// @returns true when this and @p other can be overlaid, defined as:
    /// (1) For any stage *s*, at least one of OpInfo at *s* of this or @p other is dead.
    /// (2) Except for the write-after-read case that:
    ///     For example
    ///            P 0 1 2 3 4 D
    ///        A   W L L R D D D
    ///        B   D D D W L L R
    ///    A and B can be overlaid because the Write at stage 2 of B actually happens after
    ///    the Read of A at stage 2, i.e., for write, we require Dead in the next stage, instead
    ///    of the corresponding stage. Note that, we cannot overlay in this case:
    ///            P 0 1 2  3 4 D
    ///        A   W L L R  D D D
    ///        B   D D W RW L L R
    /// TODO: we do not need to use this function during PHV allocation because
    /// AllocSlices already have the presice live range info already.
    /// This function is kept here for future overlay analysis before PHV allocation.
    bool can_overlay(const LiveRangeInfo &other) const;

    /// @returns a vector of disjoint live ranges. For example
    ///         P 0 1 2 3 4 ... D
    ///   Foo   W R D W L L  L  R
    ///   Foo.disjoint_ranges() = [(-1W, 0R), (2W, 12R)] // if tofino
    /// stage of parse is -1, mau stages starts from 0 and deparser is device::max_stage().
    /// Uninitialized reads, that are not caught by parser implicit init or
    /// when auto-init-metadata are not enabled, will have a short live range [xR, xR].
    /// These short live ranges should be treated as overlayable to any other live ranges.
    /// contract: for every paired def and use, all stages in between are marked as LIVE.
    /// NOTE: Even if IDs of units seem to be using the same schema as min-stage-based
    /// StageAndAccess, they are not. The previous StageAndAccess use max_min_stage + 1
    /// as deparser, which was very error-prone because dependency graph can be changed.
    std::vector<LiveRange> disjoint_ranges() const;

    /// @returns a new vector of live ranges that consecutive invalid live ranges will be
    /// merged into one. For example,
    /// {(1R, 1R), (2R, 2R), (5R, 5R), (6W, 10R)} => {(1R, 5R), (6W, 10R)}
    /// {(1R, 1W), (2W, 2W), (3W, 3W)} => {(1R, 1W), (2W, 3W)}
    /// premise: @p ranges must be disjoint and sorted in increasing order of LiveRange.
    /// a compiler BUG will be thrown if not satisfied.
    static std::vector<LiveRange> merge_invalid_ranges(const std::vector<LiveRange> &ranges);
};

LiveRangeInfo::OpInfo operator|(const LiveRangeInfo::OpInfo &, const LiveRangeInfo::OpInfo &);
LiveRangeInfo::OpInfo &operator|=(LiveRangeInfo::OpInfo &, const LiveRangeInfo::OpInfo &);

std::ostream &operator<<(std::ostream &out, const LiveRangeInfo::OpInfo &opinfo);
std::ostream &operator<<(std::ostream &out, const LiveRangeInfo &lr);

class IFieldSliceLiveRangeDB {
 public:
    /// @returns a const pointer to live range info, nullptr if not exist.
    virtual const LiveRangeInfo *get_liverange(const PHV::FieldSlice &) const = 0;
    /// @returns a default live range that should live from parser to deparser.
    virtual const LiveRangeInfo *default_liverange() const = 0;
};

/// FieldSliceLiveRangeDB compute and save the physical liverange for fieldslices.
/// Premise:
///  (1) when auto-init-metadata is disabled, all fields that may read implicit parser init def
///      must be annotated with pa_no_init and saved in pragmas.pa_no_init().getFields().
class FieldSliceLiveRangeDB : public IFieldSliceLiveRangeDB, public PassManager {
    class MapFieldSliceToAction : public Inspector {
        const PhvInfo &phv;
        const ReductionOrInfo &red_info;
        Visitor::profile_t init_apply(const IR::Node *root) override;

     public:
        /// Any OperandInfo (Field) within the set will have been written in the
        /// key action.
        ordered_map<const IR::MAU::Action *, ordered_set<PHV::FieldSlice>> action_to_writes;

        /// Any OperandInfo (Field) within the set will have been read in the
        /// key action.
        ordered_map<const IR::MAU::Action *, ordered_set<PHV::FieldSlice>> action_to_reads;

        bool preorder(const IR::MAU::Action *act) override;
        MapFieldSliceToAction(const PhvInfo &phv, const ReductionOrInfo &ri)
            : phv(phv), red_info(ri) {}
    };

    class DBSetter : public Inspector {
        struct Location {
            enum unit { PARSER, TABLE, DEPARSER };
            unit u;
            ordered_set<int> stages;
        };
        const PhvInfo &phv;
        const ClotInfo &clot;
        const MauBacktracker *backtracker;
        const FieldDefUse *defuse;
        const MapFieldSliceToAction *fs_action_map;
        FieldSliceLiveRangeDB &self;
        const PHV::Pragmas &pragmas;

        /// @returns the Location of @p unit.
        /// When is_read is true, then for fields marked as non_deparsed,
        /// locations of deparser units will be std::nullopt;
        /// When is_read is false, then for fields marked as no_implicit_init,
        /// locations of parser implicit init unit will be std::nullopt,
        /// and for fields marked as not_parsed_fields(), locations of parser unit will be none.
        std::optional<Location> to_location(const PHV::Field *field,
                                            const FieldDefUse::locpair &loc, bool is_read) const;

        /// update @p liverange based on @p loc and @p is_read.
        /// @returns the range of stages (including parde) that has been updated. Indexes of stages
        /// are following the LiveRangeInfo class:
        /// -1 = parser, numStages() = deparser, mau stages are mapped by stage numbers.
        std::pair<int, int> update_live_status(LiveRangeInfo &liverange, const Location &loc,
                                               bool is_read) const;

        /// update @p liverange based on paired input: @p use_loc and @p def_loc.
        /// return false if invalid live range is found.
        void update_live_range_info(const PHV::FieldSlice &fs, const Location &use_loc,
                                    const Location &def_loc, LiveRangeInfo &liverange) const;

        /// @returns fields that are marked as not deparsed in pragmas.
        const ordered_set<const PHV::Field *> &not_implicit_init_fields() const {
            return pragmas.pa_no_init().getFields();
        }

        /// a entry point to calculate the physical live range
        void end_apply() override;

     public:
        DBSetter(const PhvInfo &phv, const ClotInfo &clot, const MauBacktracker *backtracker,
                 const FieldDefUse *defuse, const MapFieldSliceToAction *fs_action_map,
                 FieldSliceLiveRangeDB &self, const PHV::Pragmas &pragmas)
            : phv(phv),
              clot(clot),
              backtracker(backtracker),
              defuse(defuse),
              fs_action_map(fs_action_map),
              self(self),
              pragmas(pragmas) {}
    };

    const PHV::Pragmas &pragmas;
    MapFieldSliceToAction *fs_action_map;
    DBSetter *setter;

    ordered_map<const PHV::Field *, ordered_map<PHV::FieldSlice, LiveRangeInfo>> live_range_map;
    Visitor::profile_t init_apply(const IR::Node *root) override;

    void set_liverange(const PHV::FieldSlice &, const LiveRangeInfo &);

 public:
    FieldSliceLiveRangeDB(const MauBacktracker *backtracker, const FieldDefUse *defuse,
                          const PhvInfo &phv, const ReductionOrInfo &red_info, const ClotInfo &clot,
                          const PHV::Pragmas &pragmas)
        : pragmas(pragmas) {
        fs_action_map = new MapFieldSliceToAction(phv, red_info);
        setter = new DBSetter(phv, clot, backtracker, defuse, fs_action_map, *this, pragmas);
        addPasses({fs_action_map, setter});
    }
    /// @returns a const pointer to live range info, nullptr if not exist.
    const LiveRangeInfo *get_liverange(const PHV::FieldSlice &) const override;
    /// @returns default liverange that live from parser to deparser.
    const LiveRangeInfo *default_liverange() const override;
};

}  // namespace PHV

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_FIELDSLICE_LIVE_RANGE_H_ */
