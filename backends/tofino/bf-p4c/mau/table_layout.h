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

#ifndef BF_P4C_MAU_TABLE_LAYOUT_H_
#define BF_P4C_MAU_TABLE_LAYOUT_H_

#include "bf-p4c/mau/action_format.h"
#include "bf-p4c/mau/attached_output.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "lib/safe_vector.h"

namespace StageFlag {
enum StageFlag_t { Immediate = 1, NoImmediate = 2 };
}

using namespace P4;

class LayoutOption {
 public:
    IR::MAU::Table::Layout layout;
    IR::MAU::Table::Way way;
    safe_vector<int> way_sizes;
    safe_vector<int> partition_sizes;
    safe_vector<int> dleft_hash_sizes;
    int entries = 0;
    int srams = 0, maprams = 0, tcams = 0;
    int lambs = 0;
    int local_tinds = 0;
    int select_bus_split = -1;
    int action_format_index = -1;
    bool previously_widened = false;
    bool identity = false;
    LayoutOption() {}
    explicit LayoutOption(const IR::MAU::Table::Layout l, int i)
        : layout(l), action_format_index(i) {}
    LayoutOption(const IR::MAU::Table::Layout l, const IR::MAU::Table::Way w, int i)
        : layout(l), way(w), action_format_index(i) {}
    LayoutOption *clone() const;
    void clear_mems() {
        srams = 0;
        lambs = 0;
        local_tinds = 0;
        maprams = 0;
        tcams = 0;
        entries = 0;
        select_bus_split = -1;
        way_sizes.clear();
        partition_sizes.clear();
        dleft_hash_sizes.clear();
    }

    int logical_tables() const {
        if (partition_sizes.size() > 0)
            return static_cast<int>(partition_sizes.size());
        else if (dleft_hash_sizes.size() > 0)
            return static_cast<int>(dleft_hash_sizes.size());
        return 1;
    }
    friend std::ostream &operator<<(std::ostream &, const LayoutOption &);
    void dbprint_multiline() const {}
};
inline std::ostream &operator<<(std::ostream &out, const LayoutOption *lo) {
    if (lo) return out << *lo;
    return out << "(null LayoutOption)";
}

class LayoutChoices {
    PhvInfo &phv;                 // may need to add TempVars as part of action/table
    SplitAttachedInfo &att_info;  // rewrites for splitting

 public:
    const ReductionOrInfo &red_info;  // needed by action analysis
    FindPayloadCandidates fpc;

 private:
    virtual void setup_exact_match(const IR::MAU::Table *tbl,
                                   const IR::MAU::Table::Layout &layout_proto,
                                   ActionData::FormatType_t format_type,
                                   int action_data_bytes_in_table, int immediate_bits, int index);
    virtual void setup_layout_option_no_match(const IR::MAU::Table *tbl,
                                              const IR::MAU::Table::Layout &layout,
                                              ActionData::FormatType_t format_type);
    virtual void setup_ternary_layout(const IR::MAU::Table *tbl,
                                      const IR::MAU::Table::Layout &layout_proto,
                                      ActionData::FormatType_t format_type,
                                      int action_data_bytes_in_table, int immediate_bits,
                                      int index);
    void compute_action_formats(const IR::MAU::Table *t, ActionData::FormatType_t type);
    void compute_layout_options(const IR::MAU::Table *t, ActionData::FormatType_t type);

    void add_hash_action_option(const IR::MAU::Table *tbl, const IR::MAU::Table::Layout &layout,
                                ActionData::FormatType_t format_type, bool &hash_action_only);
    static void setup_indirect_ptrs(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl,
                                    ActionData::FormatType_t format_type);
    void setup_layout_options(const IR::MAU::Table *tbl, const IR::MAU::Table::Layout &layout_proto,
                              ActionData::FormatType_t format_type);
    void setup_ternary_layout_options(const IR::MAU::Table *tbl,
                                      const IR::MAU::Table::Layout &layout_proto,
                                      ActionData::FormatType_t format_type);
    bool need_meter(const IR::MAU::Table *t, ActionData::FormatType_t format_type) const;

 protected:
    using key_t = std::pair<cstring, ActionData::FormatType_t>;
    template <class T>
    using cache_t = std::map<key_t, safe_vector<T>>;
    cache_t<LayoutOption> cache_layout_options;
    cache_t<ActionData::Format::Use> cache_action_formats;
    int get_pack_pragma_val(const IR::MAU::Table *tbl, const IR::MAU::Table::Layout &layout_proto);

 public:
    const safe_vector<LayoutOption> &get_layout_options(const IR::MAU::Table *t,
                                                        ActionData::FormatType_t type) {
        BUG_CHECK(t, "null table pointer");
        auto key = std::make_pair(t->name, type);
        if (!cache_layout_options.count(key)) compute_layout_options(t, type);
        return cache_layout_options.at(key);
    }
    const safe_vector<LayoutOption> &get_layout_options(const IR::MAU::Table *t) {
        return get_layout_options(t, ActionData::FormatType_t::default_for_table(t));
    }

    const safe_vector<ActionData::Format::Use> &get_action_formats(const IR::MAU::Table *t,
                                                                   ActionData::FormatType_t type) {
        BUG_CHECK(t, "null table pointer");
        auto key = std::make_pair(t->name, type);
        if (!cache_action_formats.count(key)) compute_action_formats(t, type);
        return cache_action_formats.at(key);
    }
    const safe_vector<ActionData::Format::Use> &get_action_formats(const IR::MAU::Table *t) {
        return get_action_formats(t, ActionData::FormatType_t::default_for_table(t));
    }

    // meter output formats are stored here, but they are essentially completely independent
    // of the layout choices.
    std::map<cstring /* table name */, MeterALU::Format::Use> total_meter_output_format;
    MeterALU::Format::Use get_attached_formats(const IR::MAU::Table *t,
                                               ActionData::FormatType_t format_type) const {
        if (!t || !total_meter_output_format.count(t->name) || !need_meter(t, format_type))
            return {};
        return total_meter_output_format.at(t->name);
    }

    void clear() {
        cache_layout_options.clear();
        cache_action_formats.clear();
        total_meter_output_format.clear();
        fpc.clear();
    }

    static LayoutChoices *create(PhvInfo &p, const ReductionOrInfo &ri, SplitAttachedInfo &a);
    LayoutChoices(PhvInfo &p, const ReductionOrInfo &ri, SplitAttachedInfo &a)
        : phv(p), att_info(a), red_info(ri), fpc(phv) {}
    void add_payload_gw_layout(const IR::MAU::Table *tbl, const LayoutOption &base_option);
};

/** Checks to see if the action(s) have hash distribution or rng access somewhere */
class GetActionRequirements : public MauInspector {
    bool _hash_dist_needed = false;
    bool _rng_needed = false;
    bool preorder(const IR::MAU::HashDist *) {
        _hash_dist_needed = true;
        return false;
    }
    bool preorder(const IR::MAU::RandomNumber *) {
        _rng_needed = true;
        return false;
    }

 public:
    bool is_hash_dist_needed() { return _hash_dist_needed; }
    bool is_rng_needed() { return _rng_needed; }
};

class RandomExternUsedOncePerAction : public MauInspector {
    using RandExterns = ordered_set<const IR::MAU::RandomNumber *>;
    using RandKey = std::pair<const IR::MAU::Table *, const IR::MAU::Action *>;
    ordered_map<RandKey, RandExterns> rand_extern_per_action;
    void postorder(const IR::MAU::RandomNumber *rn) override;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = MauInspector::init_apply(node);
        rand_extern_per_action.clear();
        return rv;
    }

 public:
    RandomExternUsedOncePerAction() {}
};

class MeterColorMapramAddress : public PassManager {
    ordered_map<const IR::MAU::Table *, bitvec> occupied_buses;
    ordered_map<const IR::MAU::Meter *, bitvec> possible_addresses;
    const SplitAttachedInfo &att_info;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = PassManager::init_apply(node);
        occupied_buses.clear();
        possible_addresses.clear();
        return rv;
    }

    class FindBusUsers : public MauInspector {
        MeterColorMapramAddress &self;
        bool preorder(const IR::MAU::IdleTime *) override;
        bool preorder(const IR::MAU::Counter *) override;
        // Don't want to visit AttachedOutputs/StatefulCall
        bool preorder(const IR::MAU::Action *) override { return false; }

     public:
        explicit FindBusUsers(MeterColorMapramAddress &self) : self(self) {}
    };

    class DetermineMeterReqs : public MauInspector {
        MeterColorMapramAddress &self;
        bool preorder(const IR::MAU::Meter *) override;
        // Don't want to visit AttachedOutputs/StatefulCall
        bool preorder(const IR::MAU::Action *) override { return false; }

     public:
        explicit DetermineMeterReqs(MeterColorMapramAddress &self) : self(self) {}
    };

    class SetMapramAddress : public MauModifier {
        MeterColorMapramAddress &self;
        bool preorder(IR::MAU::Meter *) override;
        // Don't want to visit AttachedOutputs/StatefulCall
        bool preorder(IR::MAU::Action *) override { return false; }

     public:
        explicit SetMapramAddress(MeterColorMapramAddress &self) : self(self) {}
    };

 public:
    explicit MeterColorMapramAddress(const SplitAttachedInfo &att_info) : att_info(att_info) {
        addPasses(
            {new FindBusUsers(*this), new DetermineMeterReqs(*this), new SetMapramAddress(*this)});
    }
};

class ValidateActionProfileFormat : public MauInspector {
    LayoutChoices &lc;

    bool preorder(const IR::MAU::ActionData *) override;

 public:
    explicit ValidateActionProfileFormat(LayoutChoices &l) : lc(l) { visitDagOnce = false; }
};

class ValidateTableSize : public MauInspector {
    bool preorder(const IR::MAU::Table *) override;

 public:
    ValidateTableSize() {}
};

class ProhibitAtcamWideSelectors : public MauInspector {
    bool preorder(const IR::MAU::Table *) override {
        visitOnce();
        return true;
    }
    bool preorder(const IR::MAU::Selector *) override;

 public:
    ProhibitAtcamWideSelectors() { visitDagOnce = false; }
};

class CheckPlacementPriorities : public MauInspector {
    ordered_map<cstring, std::set<cstring>> placement_priorities;
    bool run_once = false;

    profile_t init_apply(const IR::Node *root) override {
        auto rv = MauInspector::init_apply(root);
        placement_priorities.clear();
        return rv;
    }

    bool preorder(const IR::MAU::Table *tbl) override;
    void end_apply() override;

 public:
    CheckPlacementPriorities() {}
};

class TableLayout : public PassManager {
    LayoutChoices &lc;
    SplitAttachedInfo &att_info;

    profile_t init_apply(const IR::Node *root) override;

 public:
    TableLayout(PhvInfo &p, LayoutChoices &l, SplitAttachedInfo &sia);
    static void check_for_ternary(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl);
    static void check_for_atcam(IR::MAU::Table::Layout &layout, const IR::MAU::Table *tbl,
                                cstring &partition_index, const PhvInfo &phv);
    static void check_for_alpm(IR::MAU::Table::Layout &, const IR::MAU::Table *tbl,
                               cstring &partition_index, const PhvInfo &phv);
};

/// Run after TablePlacement to assign LR(t) values for counters.
/// Collect RAMs used by each counter in each stage.
/// For each counter in each stage, calculate LR(t) values (if necessary).
class AssignCounterLRTValues : public PassManager {
 private:
    std::map<UniqueId, int> totalCounterRams = {};

 public:
    Visitor::profile_t init_apply(const IR::Node *root) override {
        auto rv = PassManager::init_apply(root);
        totalCounterRams.clear();
        return rv;
    }

    class FindCounterRams : public MauInspector {
     private:
        AssignCounterLRTValues &self_;

     public:
        bool preorder(const IR::MAU::Table *table) override;
        explicit FindCounterRams(AssignCounterLRTValues &self) : self_(self) {}
    };

    class ComputeLRT : public MauModifier {
     private:
        AssignCounterLRTValues &self_;
        void calculate_lrt_threshold_and_interval(const IR::MAU::Table *tbl,
                                                  IR::MAU::Counter *cntr);

     public:
        bool preorder(IR::MAU::Counter *cntr) override;
        explicit ComputeLRT(AssignCounterLRTValues &self) : self_(self) {}
    };

    AssignCounterLRTValues() { addPasses({new FindCounterRams(*this), new ComputeLRT(*this)}); }
};

#endif /* BF_P4C_MAU_TABLE_LAYOUT_H_ */
