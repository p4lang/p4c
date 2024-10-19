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

#ifndef BF_P4C_MAU_ASM_OUTPUT_H_
#define BF_P4C_MAU_ASM_OUTPUT_H_

#include <initializer_list>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/next_table.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/log.h"
#include "lib/safe_vector.h"

class PhvInfo;
class BFN_Options;
namespace MauPower {
class FinalizeMauPredDepsPower;
}

struct fmt_state {
    const char *sep = " ";
    int next = 0;
    void emit(std::ostream &out, const char *name, int group, int bit, int width) {
        if (bit < 0) return;
        out << sep << name;
        if (group != -1) out << '(' << group << ")";
        out << ": ";
        out << bit << ".." << bit + width - 1;
        next = bit + width;
        sep = ", ";
    }
    void emit(std::ostream &out, const char *name, int group,
              const safe_vector<std::pair<int, int>> &bits) {
        if (bits.size() == 1) {
            emit(out, name, group, bits[0].first, bits[0].second - bits[0].first + 1);
        } else if (bits.size() > 1) {
            out << sep << name;
            if (group != -1) out << '(' << group << ")";
            out << ": [ ";
            sep = "";
            for (auto &p : bits) {
                out << sep << p.first << ".." << p.second;
                sep = ", ";
            }
            out << " ]";
        }
    }
};
cstring format_name(int type);

class memory_vector {
    const safe_vector<int> &vec;
    Memories::Use::type_t type;
    bool is_mapcol;
    friend std::ostream &operator<<(std::ostream &, const memory_vector &);

 public:
    memory_vector(const safe_vector<int> &v, Memories::Use::type_t t, bool ism)
        : vec(v), type(t), is_mapcol(ism) {}
};

class MauAsmOutput : public MauInspector {
 protected:
    const PhvInfo &phv;
    const IR::BFN::Pipe *pipe;
    const NextTable *nxt_tbl;
    const MauPower::FinalizeMauPredDepsPower *power_and_mpr;
    const BFN_Options &options;

 private:
    struct TableInstance {
        explicit TableInstance(const IR::MAU::Table *table) : tableInfo(table) {}

        const IR::MAU::Table *tableInfo;
    };

    std::map<std::pair<gress_t, int>, std::vector<TableInstance>> by_stage;
    ordered_map<std::pair<int, const IR::MAU::Selector *>,
                std::pair<UniqueId, const Memories::Use *>>
        selector_memory;
    profile_t init_apply(const IR::Node *root) override { return MauInspector::init_apply(root); }
    bool preorder(const IR::MAU::Table *tbl) override {
        auto tableId = std::make_pair(tbl->gress, tbl->stage());
        by_stage[tableId].push_back(TableInstance(tbl));
        return true;
    }
    bool preorder(const IR::MAU::BackendAttached *) override {
        // Same backend resource can be attached to a table split across
        // multiple stages which means it must be visited for each table.
        // In order to enable this, we need to set visitAgain on the common
        // resource
        visitAgain();
        return true;
    }
    void postorder(const IR::MAU::Selector *as) override {
        visitAgain();
        auto tbl = findContext<IR::MAU::Table>();
        if (tbl->is_placed()) {
            auto &unattached = tbl->resources->memuse.at(tbl->unique_id()).unattached_tables;
            auto unique_id = tbl->unique_id(as);
            if (!unattached.count(unique_id) && tbl->resources->memuse.count(unique_id)) {
                selector_memory[std::make_pair(tbl->stage(), as)] =
                    std::make_pair(unique_id, &tbl->resources->memuse.at(unique_id));
            }
        }
    }
    bool preorder(const IR::MAU::StatefulAlu *) override { return false; }
    friend std::ostream &operator<<(std::ostream &, const MauAsmOutput &);

 public:
    class NextTableSet;

 protected:
    bool require_ixbar(const IR::MAU::Table *tbl, IXBar::Use::type_t) const;
    bool require_ixbar(const IR::MAU::Table *tbl, std::initializer_list<IXBar::Use::type_t>) const;
    void emit_ixbar(std::ostream &out, indent_t indent, const IR::MAU::Table *tbl,
                    IXBar::Use::type_t type) const;
    void emit_ixbar(std::ostream &out, indent_t indent, const IR::MAU::Table *tbl,
                    std::initializer_list<IXBar::Use::type_t> types) const;
    void emit_random_seed(std::ostream &out, indent_t indent, const TableMatch *fmt) const;
    // FIXME: change API to be target-agnostic
    void emit_hash_dist(std::ostream &out, indent_t indent,
                        const safe_vector<Tofino::IXBar::HashDistUse> *hash_dist_use,
                        bool hashmod) const;

    bool emit_gateway(std::ostream &out, indent_t gw_indent, const IR::MAU::Table *tbl,
                      bool hash_action, NextTableSet next_hit, NextTableSet &gw_miss) const;
    void emit_no_match_gateway(std::ostream &out, indent_t gw_indent,
                               const IR::MAU::Table *tbl) const;
    void emit_table_hitmap(std::ostream &out, indent_t indent, const IR::MAU::Table *tbl,
                           NextTableSet &next_hit, NextTableSet &gw_miss, bool no_match_hit,
                           bool gw_can_miss) const;

    virtual void emit_ways(std::ostream &out, indent_t indent, const IXBar::Use *use,
                           const Memories::Use *mem) const;

 public:
    virtual void emit_table_format(std::ostream &out, indent_t, const TableFormat::Use &use,
                                   const TableMatch *tm, bool ternary, bool no_match) const;

 protected:
    virtual void emit_memory(std::ostream &out, indent_t, const Memories::Use &,
                             const IR::MAU::Table::Layout *l = nullptr,
                             const TableFormat::Use *f = nullptr) const = 0;

    void emit_table_context_json(std::ostream &out, indent_t, const IR::MAU::Table *tbl) const;
    void emit_ternary_match(std::ostream &out, indent_t, const TableFormat::Use &use) const;
    void emit_atcam_match(std::ostream &out, indent_t, const IR::MAU::Table *tbl,
                          std::stringstream &context_json_entries) const;
    void emit_table(std::ostream &out, const IR::MAU::Table *tbl, int stage, gress_t gress) const;
    void emit_always_run_action(std::ostream &out, const IR::MAU::Table *tbl, int stage,
                                gress_t gress) const;
    void emit_static_entries(std::ostream &, indent_t indent, const IR::MAU::Table *tbl,
                             std::stringstream &context_json_entries) const;
    void next_table_non_action_map(const IR::MAU::Table *,
                                   safe_vector<NextTableSet> &next_table_map) const;
    void emit_table_indir(std::ostream &out, indent_t, const IR::MAU::Table *tbl,
                          const IR::MAU::TernaryIndirect *ti) const;
    void emit_action_data_format(std::ostream &out, indent_t, const IR::MAU::Table *tbl,
                                 const IR::MAU::Action *af) const;
    void emit_single_alias(std::ostream &out, std::string &sep, const ActionData::Parameter *param,
                           le_bitrange adt_range, cstring alias,
                           safe_vector<ActionData::Argument> &full_args, cstring action_name) const;
    void emit_action_data_alias(std::ostream &out, indent_t, const IR::MAU::Table *tbl,
                                const IR::MAU::Action *af) const;
    void emit_action_data_bus(std::ostream &out, indent_t, const IR::MAU::Table *tbl,
                              bitvec source) const;
    bool emit_idletime(std::ostream &out, indent_t indent, const IR::MAU::Table *tbl,
                       const IR::MAU::IdleTime *id) const;
    void emit_indirect_res_context_json(std::ostream &, indent_t indent, const IR::MAU::Table *tbl,
                                        std::stringstream &context_json_entries) const;
    virtual bool gateway_uses_inhibit_index(const IR::MAU::Table *) const { return false; }
    std::string indirect_address(const IR::MAU::AttachedMemory *) const;
    std::string indirect_pfe(const IR::MAU::AttachedMemory *) const;
    std::string stateful_counter_addr(IR::MAU::StatefulUse use) const;
    std::string build_call(const IR::MAU::AttachedMemory *at_mem,
                           const IR::MAU::BackendAttached *ba, const IR::MAU::Table *tbl) const;
    std::string build_sel_len_call(const IR::MAU::Selector *as) const;
    std::string build_meter_color_call(const IR::MAU::Meter *mtr,
                                       const IR::MAU::BackendAttached *ba,
                                       const IR::MAU::Table *tbl) const;
    NextTableSet next_for(const IR::MAU::Table *tbl, cstring what) const;

    class EmitAction;
    class EmitAlwaysRunAction;
    class EmitAttached;
    class UnattachedName;
    // class EmitHashExpression;

 public:
    MauAsmOutput(const PhvInfo &phv, const IR::BFN::Pipe *pipe, const NextTable *nxts,
                 const MauPower::FinalizeMauPredDepsPower *pmpr, const BFN_Options &options)
        : phv(phv), pipe(pipe), nxt_tbl(nxts), power_and_mpr(pmpr), options(options) {}

    static cstring find_attached_name(const IR::MAU::Table *tbl, const IR::MAU::AttachedMemory *am);
    static ordered_set<UniqueId> find_attached_ids(const IR::MAU::Table *tbl,
                                                   const IR::MAU::AttachedMemory *am);
};

class TableMatch {
 public:
    const IR::MAU::Table *table = nullptr;
    const PhvInfo &phv;

    // 'match_fields', 'proxy_hash'  and 'proxy_hash_fields' here are used only by
    // 'emit_table_format' while 'ghost_bits' and 'identity_hash' are used only by
    // 'emit_ixbar_hash_table' (called via emit_ixbar and emit_single_ixbar), so they
    // could be two independent data structures (or just extra arguments) -- the combo
    // here into a single object is accidental.
    safe_vector<Slice> match_fields;
    safe_vector<Slice> match_fields_word;
    safe_vector<Slice> match_fields_byte;
    safe_vector<Slice> ghost_bits;

    struct ProxyHashSlice {
        le_bitrange bits;
        explicit ProxyHashSlice(le_bitrange b) : bits(b) {}

     public:
        friend std::ostream &operator<<(std::ostream &out, const ProxyHashSlice &sl) {
            return out << "hash_group" << "(" << sl.bits.lo << ".." << sl.bits.hi << ")";
        }
    };
    safe_vector<ProxyHashSlice> proxy_hash_fields;
    bool proxy_hash = false;
    bool identity_hash = false;
    bool dynamic_key_masks = false;

    void init_proxy_hash();
    virtual void populate_match_fields();
    void populate_ghost_bits();
    void populate_slices(safe_vector<Slice> &slices,
                         const std::map<IXBar::Use::Byte, bitvec> &byte_infos);

    TableMatch(const PhvInfo &phv, const IR::MAU::Table *tbl);
    explicit TableMatch(const PhvInfo &phv) : phv(phv) {}
    static TableMatch *create(const PhvInfo &phv, const IR::MAU::Table *tbl);
};

#endif /* BF_P4C_MAU_ASM_OUTPUT_H_ */
