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

#include "bf-p4c/phv/v2/greedy_tx_score.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <utility>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/kind_size_indexed_map.h"
#include "bf-p4c/phv/v2/sort_macros.h"

namespace PHV {
namespace v2 {

using ContGress = Vision::ContGress;

namespace {

/// convert slices to a bitvec where 1 represents an occupied bit.
bitvec make_allocated_bitvec(const ordered_set<PHV::AllocSlice> &slices) {
    bitvec allocated;
    for (const auto &slice : slices) {
        allocated.setrange(slice.container_slice().lo, slice.container_slice().size());
    }
    return allocated;
}

ContGress from(const gress_t &t) {
    switch (t) {
        case INGRESS: {
            return ContGress::ingress;
        }
        case EGRESS: {
            return ContGress::egress;
        }
        case GHOST: {
            return ContGress::ingress;
        }
        default:
            BUG("unknown gress: ", t);
    }
}

// ContGress from(const PHV::Allocation::GressAssignment& t) {
//     if (!t) return ContGress::unassigned;
//     return from(*t);
// }

ContGress device_gress(const Container &c) {
    const auto &phv_spec = Device::phvSpec();
    if (phv_spec.ingressOnly()[phv_spec.containerToId(c)]) {
        return ContGress::ingress;
    } else if (phv_spec.egressOnly()[phv_spec.containerToId(c)]) {
        return ContGress::egress;
    } else {
        return ContGress::unassigned;
    }
}

/// call @p f with AllocSlice(s) that slices of the same field slices with different live ranges
/// will be passed to @p f only once (de-duplicate by {field, range} pair).
template <typename T>
void deduped_iterate(const T &alloc_slices, const std::function<void(const AllocSlice &)> &f) {
    ordered_set<std::pair<const PHV::Field *, le_bitrange>> seen;
    for (const auto &slice : alloc_slices) {
        auto fs = std::make_pair(slice.field(), slice.field_slice());
        if (seen.count(fs)) continue;
        seen.insert(fs);
        f(slice);
    }
}

void update_table_stage_ixbar_bytes(const PhvKit &kit, const Container &c,
                                    const ordered_set<AllocSlice> &slices,
                                    TableIxbarContBytesMap &table_ixbar_cont_bytes,
                                    StageIxbarContBytesMap &stage_sram_ixbar_cont_bytes,
                                    StageIxbarContBytesMap &stage_tcam_ixbar_cont_bytes) {
    for (const auto &sl : slices) {
        for (const auto &tb_key : kit.uses.ixbar_read(sl.field(), sl.field_slice())) {
            const auto *tb = tb_key.first;
            const auto &c_range = sl.container_slice();
            for (int i = c_range.loByte(); i <= c_range.hiByte(); i++) {
                const auto cont_byte = std::make_pair(c, i);
                table_ixbar_cont_bytes[tb].insert(cont_byte);
                for (const auto &stage : kit.mau.stage(tb, true)) {
                    if (PhvKit::is_ternary(tb)) {
                        stage_tcam_ixbar_cont_bytes[stage].insert(cont_byte);
                    } else {
                        stage_sram_ixbar_cont_bytes[stage].insert(cont_byte);
                    }
                }
            }
        }
    }
}

}  // namespace

int Vision::bits_demand(const PHV::Kind &k) const {
    int rv = 0;
    for (const auto &db : Values(cont_required)) {
        for (const auto &ks_n : db.m) {
            if (ks_n.first.first != k) continue;
            rv += static_cast<int>(ks_n.first.second) * ks_n.second;
        }
    }
    return rv;
}

int Vision::bits_supply(const PHV::Kind &k) const {
    int rv = 0;
    for (const auto &db : Values(cont_available)) {
        for (const auto &ks_n : db.m) {
            if (ks_n.first.first != k) continue;
            rv += static_cast<int>(ks_n.first.second) * ks_n.second;
        }
    }
    return rv;
}

int GreedyTxScoreMaker::ixbar_imbalanced_alignment(const ordered_set<ContainerByte> &cont_bytes) {
    std::vector<int> counters(4, 0);
    // allocate 32-bit container bytes first.
    for (const auto &cont_byte : cont_bytes) {
        if (cont_byte.first.type().size() != PHV::Size::b32) continue;
        counters[cont_byte.second]++;
    }
    // then 16-bit, greedy algorithm, pick the smaller one to allocate.
    for (const auto &cont_byte : cont_bytes) {
        if (cont_byte.first.type().size() != PHV::Size::b16) continue;
        if (counters[cont_byte.second] <= counters[cont_byte.second + 2]) {
            counters[cont_byte.second]++;
        } else {
            counters[cont_byte.second + 2]++;
        }
    }
    // larger the smallest, less the imbalanced.
    std::sort(counters.begin(), counters.end());
    const int min_pressure = std::max(1, counters.front());
    int total_imba = 0;
    for (const auto &c : counters) {
        total_imba += std::max(0, c - min_pressure);
    }
    return total_imba;
}

GreedyTxScoreMaker::GreedyTxScoreMaker(
    const PhvKit &kit, const std::list<ContainerGroup *> &container_groups,
    const std::list<SuperCluster *> &sorted_clusters,
    const ordered_map<const SuperCluster *, KindSizeIndexedMap> &baseline)
    : kit_i(kit) {
    ///// compute container kind and size indexed availability and demand.
    // initialization
    for (const auto &gress : {INGRESS, EGRESS}) {
        vision_i.cont_available[from(gress)] = KindSizeIndexedMap();
        vision_i.cont_required[gress] = KindSizeIndexedMap();
    }
    vision_i.cont_available[ContGress::unassigned] = KindSizeIndexedMap();

    // container size pressure.
    for (auto *sc : sorted_clusters) {
        vision_i.sc_cont_required[sc] = baseline.at(sc);
        auto gress = sc->slices().front().field()->gress;
        for (const auto &kv : vision_i.sc_cont_required[sc].m) {
            vision_i.cont_required[gress][kv.first] += kv.second;
        }
    }
    // container availability
    for (const auto &group : container_groups) {
        for (const auto &c : *group) {
            auto kind_sz = std::make_pair(c.type().kind(), c.type().size());
            vision_i.cont_available[device_gress(c)].m[kind_sz]++;
        }
    }

    // Record all fields that are TCAM table keys
    for (const auto &[table, prop] : kit_i.tb_keys.props()) {
        if (prop.is_tcam && prop.is_range) {
            for (const FieldSlice &fs : prop.keys) {
                table_key_with_ranges.insert(fs.field());
            }
        }
    }
}

void GreedyTxScoreMaker::record_commit(const Transaction &tx, const SuperCluster *sc) {
    using AllocStatus = Allocation::ContainerAllocStatus;
    auto *parent = tx.getParent();
    for (const auto &container_status : tx.get_actual_diff()) {
        const auto &c = container_status.first;
        const auto &slices = container_status.second.slices;
        auto parent_c_status = AllocStatus::EMPTY;
        if (auto parent_status = parent->getStatus(c)) {
            parent_c_status = parent_status->alloc_status;
        }
        // update container availability vector.
        if (parent_c_status == AllocStatus::EMPTY && !slices.empty()) {
            vision_i.cont_available[device_gress(c)][{c.type().kind(), c.type().size()}]--;
        }
        // update vision ixbar bytes stats.
        update_table_stage_ixbar_bytes(kit_i, c, slices, vision_i.table_ixbar_cont_bytes,
                                       vision_i.stage_sram_ixbar_cont_bytes,
                                       vision_i.stage_tcam_ixbar_cont_bytes);
    }

    // update cont required.
    // skip special clusters: deparser-zero, strided...
    if (vision_i.sc_cont_required.count(sc)) {
        for (const auto &kindsize_n : vision_i.sc_cont_required.at(sc).m) {
            vision_i.cont_required[sc->gress()][kindsize_n.first] -= kindsize_n.second;
        }
    }
}

KindSizeIndexedMap GreedyTxScoreMaker::record_deallocation(const SuperCluster *sc,
                                                           const ConcreteAllocation &curr_alloc,
                                                           const ordered_set<AllocSlice> &slices) {
    KindSizeIndexedMap baseline;
    ordered_map<Container, ordered_set<AllocSlice>> cont_slices;
    for (const auto &sl : slices) {
        cont_slices[sl.container()].insert(sl);
    }
    for (const auto &c_slices : cont_slices) {
        const auto &c = c_slices.first;
        const auto &slices = c_slices.second;
        const auto &status = curr_alloc.getStatus(c);
        bool no_other_cluster = true;
        for (const auto &sl : status->slices) {
            no_other_cluster &= slices.count(sl);
            if (!no_other_cluster) break;
        }
        if (no_other_cluster) {
            auto kind_size = std::make_pair(c.type().kind(), c.type().size());
            baseline.m[kind_size]++;
            vision_i.cont_available[device_gress(c)][kind_size]++;
            vision_i.cont_required[sc->gress()][kind_size]++;
        }
    }
    vision_i.sc_cont_required[sc] = baseline;

    // reset ixbar bytes info.
    vision_i.table_ixbar_cont_bytes.clear();
    vision_i.stage_sram_ixbar_cont_bytes.clear();
    vision_i.stage_tcam_ixbar_cont_bytes.clear();
    for (const auto &cont_status : curr_alloc) {
        const auto &c = cont_status.first;
        auto slices = cont_status.second.slices;
        if (cont_slices.count(c)) {
            for (const auto &erased : cont_slices.at(c)) {
                slices.erase(erased);
            }
        }
        update_table_stage_ixbar_bytes(kit_i, c, slices, vision_i.table_ixbar_cont_bytes,
                                       vision_i.stage_sram_ixbar_cont_bytes,
                                       vision_i.stage_tcam_ixbar_cont_bytes);
    }
    return baseline;
}

TxScore *GreedyTxScoreMaker::make(const Transaction &tx) const {
    auto *rv = new GreedyTxScore(&vision_i);
    const auto *parent = tx.getParent();
    std::optional<gress_t> tx_gress = std::nullopt;
    ordered_map<Vision::ContGress, KindSizeIndexedMap> newly_used;
    for (const auto &kv : tx.get_actual_diff()) {
        const auto &c = kv.first;
        const auto &slices = kv.second.slices;
        const auto &parent_slices = parent->slices(c);
        if (slices.size() == 0) continue;
        tx_gress = slices.front().field()->gress;

        for (const auto &slice : slices) {
            if (!table_key_with_ranges.count(slice.field())) continue;
            // Slices less than 4 bits should ideally not cross nibble boundaries
            const le_bitrange &range = slice.container_slice();
            int lo_nibble = range.lo / 4;
            int hi_nibble = range.hi / 4;
            rv->n_range_match_nibbles_occupied = hi_nibble - lo_nibble + 1;
        }
        const auto c_kind = c.type().kind();
        const auto c_size = c.type().size();
        const auto c_kind_size = std::make_pair(c_kind, c_size);
        const bool has_solitary =
            std::any_of(slices.begin(), slices.end(),
                        [&](const AllocSlice &sl) { return sl.field()->is_solitary(); });
        const bool no_more_packing =
            has_solitary || c_kind == PHV::Kind::mocha || c_kind == PHV::Kind::dark;

        // compute newly used_bits: (1) if no more packing, then all unused bits in parent tx are
        // counted as used, else (2) count if the bit is used.
        const bitvec parent_bv = make_allocated_bitvec(parent_slices);
        const bitvec tx_bv = make_allocated_bitvec(tx.slices(c));
        for (size_t i = c.lsb(); i <= c.msb(); i++) {
            if (!parent_bv.getbit(i)) {
                rv->used_bits[c_kind_size] += no_more_packing ? 1 : tx_bv.getbit(i);
            }
        }

        // compute newly used container.
        if (parent_bv.empty()) {
            rv->used_containers[c_kind_size]++;
            newly_used[device_gress(c)][{c.type().kind(), c.type().size()}]++;
        }

        const auto &gress = kv.second.gress;
        const auto &parser_group_gress = kv.second.parserGroupGress;
        const auto &deparser_group_gress = kv.second.deparserGroupGress;

        // gress deparser gress mismatch.
        if (!parent->gress(c) && gress) {
            rv->n_set_gress[c_kind_size]++;
            // creating a mismatch by assigning different container gress.
            if (deparser_group_gress && gress != deparser_group_gress) {
                rv->n_mismatched_deparser_gress[c_kind_size]++;
            }
        }

        // Parser group gress
        if (!parent->parserGroupGress(c) && parser_group_gress) {
            rv->n_set_parser_gress[c_kind_size]++;
        }

        // deparser group gress
        if (!parent->deparserGroupGress(c) && deparser_group_gress) {
            rv->n_set_deparser_gress[c_kind_size]++;
            const auto &phv_spec = Device::phvSpec();
            // count mismatches of assigning different gress to a deparser group of containers.
            for (unsigned other_id : phv_spec.deparserGroup(phv_spec.containerToId(c))) {
                auto other_cont = phv_spec.idToContainer(other_id);
                if (other_cont == c) continue;
                if (tx.gress(other_cont) && tx.gress(other_cont) != deparser_group_gress) {
                    rv->n_mismatched_deparser_gress[c_kind_size]++;
                }
            }
        }

        // tphv_on_phv_bits
        if (Device::phvSpec().hasContainerKind(PHV::Kind::tagalong) && c_kind != Kind::tagalong) {
            deduped_iterate(slices, [&](const AllocSlice &slice) {
                if (slice.field()->is_tphv_candidate(kit_i.uses)) {
                    rv->n_tphv_on_phv_bits += slices.size();
                }
            });
        }

        // n_delta_pov_deparser_read_bits
        const bool has_pov = std::any_of(slices.begin(), slices.end(),
                                         [&](const AllocSlice &sl) { return sl.field()->pov; });
        const bool parent_has_pov =
            std::any_of(parent_slices.begin(), parent_slices.end(),
                        [&](const AllocSlice &sl) { return sl.field()->pov; });
        if (!parent_has_pov && has_pov) {
            rv->n_pov_deparser_read_bits += c.size();
        }

        // n_deparser_read_digest_fields_bytes
        // TODO: must use this uses.is_learning because it includes selector field.
        const bool has_learning =
            std::any_of(slices.begin(), slices.end(),
                        [&](const AllocSlice &sl) { return kit_i.uses.is_learning(sl.field()); });
        if (has_learning) {
            rv->n_deparser_read_learning_bytes += c.size() / 8;
        }
    }

    // TODO: we can know how many sram group and tcam group has left in that stage,
    // especially by comparing by the baseline ixbar usage.
    ordered_set<Container> range_match_double_bytes_container;
    TableIxbarContBytesMap new_table_ixbar_cont_bytes;
    StageIxbarContBytesMap new_stage_sram_ixbar_cont_bytes;
    StageIxbarContBytesMap new_stage_tcam_ixbar_cont_bytes;
    for (const auto &kv : tx.get_actual_diff()) {
        const auto &c = kv.first;
        const auto &slices = kv.second.slices;
        update_table_stage_ixbar_bytes(kit_i, c, slices, new_table_ixbar_cont_bytes,
                                       new_stage_sram_ixbar_cont_bytes,
                                       new_stage_tcam_ixbar_cont_bytes);
        // When placing a less-than-24-bit and range look-up match key field to
        // a 32-bit container, because there are inefficiencies of the implementation
        // of ixbar allocation: it will use two TCAM groups, which will double the
        // number of bytes used.
        if (c.type().size() == PHV::Size::b32) {
            for (const auto &sl : slices) {
                if (range_match_double_bytes_container.count(c)) break;
                if (sl.field()->size >= 24) continue;
                const auto &fs = FieldSlice(sl.field(), sl.field_slice());
                for (const auto &tb_key : kit_i.uses.ixbar_read(fs.field(), fs.range())) {
                    const auto *tb = tb_key.first;
                    const auto *key = tb_key.second;
                    for (const int stage : kit_i.mau.stage(tb, true)) {
                        if (!sl.isLiveAt(stage, FieldUse(FieldUse::READ))) continue;
                        if (key->for_range()) {
                            range_match_double_bytes_container.insert(c);
                            break;
                        }
                    }
                }
            }
        }
    }
    // n_table_ixbar_imbalance_alignments and n_table_new_ixbar_bytes.
    for (auto &table_cont_bytes : new_table_ixbar_cont_bytes) {
        const auto &tb = table_cont_bytes.first;
        auto &cont_bytes = table_cont_bytes.second;
        // merge with allocated byte stats.
        if (vision_i.table_ixbar_cont_bytes.count(tb)) {
            const auto &allocated = vision_i.table_ixbar_cont_bytes.at(tb);
            for (const auto &cont_byte : cont_bytes) {
                if (!allocated.count(cont_byte)) {
                    rv->n_table_new_ixbar_bytes +=
                        range_match_double_bytes_container.count(cont_byte.first) ? 2 : 1;
                }
            }
            cont_bytes.insert(allocated.begin(), allocated.end());
        }
        const int imba = ixbar_imbalanced_alignment(cont_bytes);
        rv->n_table_ixbar_imbalanced_alignments += imba;
        LOG6("Allocated match-key container bytes of table " << tb->name);
        for (const auto &b : cont_bytes) {
            LOG6(b);
        }
        LOG6("imba score: " << imba);
    }

    constexpr int STAGE_SRAM_IXBAR_BYTES = 128;
    constexpr int STAGE_TCAM_IXBAR_BYTES = 66;
    const auto add_stage_pressure = [&](StageIxbarContBytesMap &stage_ixbar_bytes,
                                        const StageIxbarContBytesMap &stage_allocated,
                                        const int limit) {
        for (auto &stage_cont_bytes : stage_ixbar_bytes) {
            const auto &stage = stage_cont_bytes.first;
            auto &cont_bytes = stage_cont_bytes.second;
            // merge with allocated byte stats.
            if (stage_allocated.count(stage)) {
                const auto &allocated = stage_allocated.at(stage);
                for (const auto &cont_byte : cont_bytes) {
                    if (!allocated.count(cont_byte)) {
                        rv->n_stage_new_ixbar_bytes +=
                            range_match_double_bytes_container.count(cont_byte.first) ? 2 : 1;
                    }
                }
                cont_bytes.insert(allocated.begin(), allocated.end());
            }
            // when half of the bytes are used already, we think it's overloaded.
            if (int(cont_bytes.size()) > limit / 2) {
                rv->n_overloaded_stage_ixbar_imbalanced_alignments +=
                    ixbar_imbalanced_alignment(cont_bytes);
            }
        }
    };
    add_stage_pressure(new_stage_sram_ixbar_cont_bytes, vision_i.stage_sram_ixbar_cont_bytes,
                       STAGE_SRAM_IXBAR_BYTES);
    add_stage_pressure(new_stage_tcam_ixbar_cont_bytes, vision_i.stage_tcam_ixbar_cont_bytes,
                       STAGE_TCAM_IXBAR_BYTES);

    // empty tx.
    if (!tx_gress) return rv;

    // compute how many more containers is required than available, grouped by container size.
    rv->n_size_overflow = 0;
    rv->n_max_overflowed = 0;
    for (const auto s : Device::phvSpec().containerSizes()) {
        int this_size_total_overflow = 0;
        for (const auto &gress : {INGRESS, EGRESS}) {
            // we do not count size overflow Kind::tagalong, Kind::dark and because they are
            // usually more than required. Especially for TPHV, there are many headers that
            // can be overlaid.
            int required_carry_over = 0;
            for (const auto k : {Kind::mocha, Kind::normal}) {
                const int required =
                    vision_i.cont_required.at(gress).get_or(k, s, 0) + required_carry_over;
                const int this_gress_available =
                    vision_i.cont_available.at(from(gress)).get_or(k, s, 0) -
                    newly_used[from(gress)].get_or(k, s, 0);
                const int unassigned_available =
                    vision_i.cont_available.at(ContGress::unassigned).get_or(k, s, 0) -
                    newly_used[ContGress::unassigned].get_or(k, s, 0);
                // can all be allocated by gress specified containers.
                if (required <= this_gress_available) {
                    continue;
                }
                // can be allocated by using some unassigned containers.
                if (required <= this_gress_available + unassigned_available) {
                    newly_used[ContGress::unassigned][{k, s}] += (required - this_gress_available);
                    continue;
                }
                // overflowed!
                newly_used[ContGress::unassigned][{k, s}] += unassigned_available;
                const int delta = required - (this_gress_available + unassigned_available);
                if (k == Kind::normal) {
                    rv->n_size_overflow += delta;
                    this_size_total_overflow += delta;
                    rv->n_max_overflowed = std::max(rv->n_max_overflowed, this_size_total_overflow);
                } else {
                    required_carry_over += delta;
                }
            }
        }
    }

    // calc_n_inc_tphv_collections
    if (Device::currentDevice() == Device::TOFINO) {
        const auto tx_used = tx.getTagalongCollectionsUsed();
        const auto parent_used = parent->getTagalongCollectionsUsed();
        for (const auto &col : tx_used) {
            if (!parent_used.count(col)) rv->n_inc_used_tphv_collection++;
        }
    }

    return rv;
}

cstring GreedyTxScoreMaker::status() const {
    std::stringstream ss;
    ss << vision_i;
    return ss.str();
}

int GreedyTxScore::used_L1_bits() const {
    int total_used_bits = 0;
    total_used_bits += used_bits.sum(Kind::normal);
    if (!vision_i->has_more_than_enough(Kind::mocha)) {
        total_used_bits += used_bits.sum(Kind::mocha);
    }
    return total_used_bits;
}

int GreedyTxScore::used_L2_bits() const {
    int total_used_bits = 0;
    // only when we have enough mocha availability, mocha is considered to be
    // L2 bits, otherwise, it would have been counted as L1.
    if (vision_i->has_more_than_enough(Kind::mocha)) {
        total_used_bits += used_bits.sum(Kind::mocha);
    }
    if (!vision_i->has_more_than_enough(Kind::tagalong)) {
        total_used_bits += used_bits.sum(Kind::tagalong);
    }
    return total_used_bits;
}

bool GreedyTxScore::better_than(const TxScore *other_score) const {
    if (other_score == nullptr) return true;
    const GreedyTxScore *other = dynamic_cast<const GreedyTxScore *>(other_score);
    BUG_CHECK(other, "comparing GreedyTxScore with score of different type: %1%",
              other_score->str());
    BUG_CHECK(vision_i == other->vision_i, "comparing with different maker vision");

    // For tofino1 only, generally prefer to place things to tphv as long as we have
    // enough tphv container available.
    IF_NEQ_RETURN_IS_LESS(n_tphv_on_phv_bits, other->n_tphv_on_phv_bits);

    // TODO: The priority of these two metric can be flexible.
    // For now we just set it to be the highest to avoid exceeding deparser limit.
    IF_NEQ_RETURN_IS_LESS(n_pov_deparser_read_bits, other->n_pov_deparser_read_bits);
    IF_NEQ_RETURN_IS_LESS(n_deparser_read_learning_bytes, other->n_deparser_read_learning_bytes);

    // Avoid using more ixbar bytes. This has the highest priority because we
    // if table allocation failed, this PHV allocation, even if fit, would become
    // useless. In the future, if we could have some post-phv-alloc optimization
    // passes for ixbar usages, maybe we could lower the priority.
    // less ixbar bytes.
    IF_NEQ_RETURN_IS_LESS(n_table_new_ixbar_bytes, other->n_table_new_ixbar_bytes);
    IF_NEQ_RETURN_IS_LESS(n_stage_new_ixbar_bytes, other->n_stage_new_ixbar_bytes);

    // container balance has the second highest priority.
    IF_NEQ_RETURN_IS_LESS(n_size_overflow, other->n_size_overflow);
    IF_NEQ_RETURN_IS_LESS(n_max_overflowed, other->n_max_overflowed);

    // less imbalanced ixbar alignments.
    IF_NEQ_RETURN_IS_LESS(n_table_ixbar_imbalanced_alignments,
                          other->n_table_ixbar_imbalanced_alignments);
    IF_NEQ_RETURN_IS_LESS(n_overloaded_stage_ixbar_imbalanced_alignments,
                          other->n_overloaded_stage_ixbar_imbalanced_alignments);

    // compare l1 bits usage.
    const int l1_bits = used_L1_bits();
    const int other_l1_bits = other->used_L1_bits();
    IF_NEQ_RETURN_IS_LESS(l1_bits, other_l1_bits);

    // compare l2 bits usage.
    const int l2_bits = used_L2_bits();
    const int other_l2_bits = other->used_L2_bits();
    IF_NEQ_RETURN_IS_LESS(l2_bits, other_l2_bits);

    // compare newly used containers, less means more packing.
    IF_NEQ_RETURN_IS_LESS(used_containers.sum(Kind::normal),
                          other->used_containers.sum(Kind::normal));

    // less gress setting is preferred.
    IF_NEQ_RETURN_IS_LESS(n_set_gress.sum(Kind::normal), other->n_set_gress.sum(Kind::normal));
    IF_NEQ_RETURN_IS_LESS(n_set_deparser_gress.sum(Kind::normal),
                          other->n_set_deparser_gress.sum(Kind::normal));
    IF_NEQ_RETURN_IS_LESS(n_mismatched_deparser_gress.sum(Kind::normal),
                          other->n_mismatched_deparser_gress.sum(Kind::normal));
    IF_NEQ_RETURN_IS_LESS(n_set_parser_gress.sum(Kind::normal),
                          other->n_set_parser_gress.sum(Kind::normal));

    // less tphv collection use (same as gress setting).
    IF_NEQ_RETURN_IS_LESS(n_inc_used_tphv_collection, other->n_inc_used_tphv_collection);

    IF_NEQ_RETURN_IS_LESS(n_range_match_nibbles_occupied, other->n_range_match_nibbles_occupied);

    return false;
}

bool GreedyTxScore::has_mismatch_gress() const {
    return (n_mismatched_deparser_gress.sum(Kind::normal) > 0);
}

std::string GreedyTxScore::str() const {
    std::stringstream ss;
    ss << "greedy{ ";
    if (n_tphv_on_phv_bits) {
        ss << "tphv_on_phv: " << n_tphv_on_phv_bits << " ";
    }
    if (n_pov_deparser_read_bits) {
        ss << "pov_read: " << n_pov_deparser_read_bits << " ";
    }
    if (n_deparser_read_learning_bytes) {
        ss << "learning_read: " << n_deparser_read_learning_bytes << " ";
    }
    if (n_table_new_ixbar_bytes) {
        ss << "tb_ixbar: " << n_table_new_ixbar_bytes << " ";
    }
    if (n_stage_new_ixbar_bytes) {
        ss << "st_ixbar: " << n_stage_new_ixbar_bytes << " ";
    }
    ss << "ovf: " << n_size_overflow << " ";
    ss << "ovf_max: " << n_max_overflowed << " ";
    if (n_table_ixbar_imbalanced_alignments) {
        ss << "tb_ixbar_imba: " << n_table_ixbar_imbalanced_alignments << " ";
    }
    if (n_overloaded_stage_ixbar_imbalanced_alignments) {
        ss << "st_ixbar_imba: " << n_overloaded_stage_ixbar_imbalanced_alignments << " ";
    }
    ss << "L1: " << used_L1_bits() << " ";
    ss << "L2: " << used_L2_bits() << " ";
    ss << "inc_cont: " << used_containers.sum(Kind::normal) << " ";
    if (auto set_gress = n_set_gress.sum(Kind::normal)) {
        ss << "set_gress: " << set_gress << " ";
    }
    if (n_inc_used_tphv_collection) {
        ss << "tphv_col_inc: " << n_inc_used_tphv_collection << " ";
    }
    if (auto mismatch = n_mismatched_deparser_gress.sum(Kind::normal)) {
        ss << "mismatch_gress: " << mismatch << " ";
    }
    ss << "}";
    return ss.str();
}

std::ostream &operator<<(std::ostream &out, const ContainerByte &c) {
    out << c.first << "@" << c.second;
    return out;
}

std::ostream &operator<<(std::ostream &out, const KindSizeIndexedMap &m) {
    out << "{";
    std::string sep = "";
    for (const auto &kv : m.m) {
        out << sep << kv.first.first << kv.first.second << ": " << kv.second;
        sep = ", ";
    }
    out << "}";
    return out;
}

std::ostream &operator<<(std::ostream &out, const Vision &v) {
    out << "Vision {\n";
    out << " overall container bits status: " << "\n";
    auto kinds = std::vector<PHV::Kind>{Kind::mocha};
    if (Device::currentDevice() == Device::TOFINO) {
        kinds = {Kind::tagalong};
    }
    for (const auto &k : kinds) {
        out << "  " << k << "'s demand v.s. supply: " << v.bits_demand(k) << "/" << v.bits_supply(k)
            << "\n";
    }
    out << " container demand v.s. supply: \n";
    for (const auto &gress : {gress_t::INGRESS, gress_t::EGRESS}) {
        for (const auto k : {Kind::tagalong, Kind::dark, Kind::mocha, Kind::normal}) {
            for (const auto s : {PHV::Size::b8, PHV::Size::b16, PHV::Size::b32}) {
                auto n_req = v.cont_required.at(gress).get(k, s);
                if (k == Kind::normal && !n_req) n_req = 0;  // always print normal availability
                if (n_req) {
                    out << "  " << gress << "-" << k << s << ": " << *n_req << "/("
                        << v.cont_available.at(from(gress)).get_or(k, s, 0) << " + "
                        << v.cont_available.at(ContGress::unassigned).get_or(k, s, 0) << ")\n";
                }
            }
        }
    }
    std::stringstream ss;
    std::vector<std::string> headers = {"Type"};
    for (auto i = 0; i < Device::numStages(); i++) headers.push_back(std::to_string(i));
    TablePrinter tp(ss, headers, TablePrinter::Align::CENTER);
    for (const auto &t : {std::string("SRAM"), std::string("TCAM")}) {
        std::vector<std::string> row;
        row.push_back(t);
        const StageIxbarContBytesMap *bytes = &v.stage_sram_ixbar_cont_bytes;
        if (t == "TCAM") {
            bytes = &v.stage_tcam_ixbar_cont_bytes;
        }
        for (auto i = 0; i < Device::numStages(); i++) {
            row.push_back(std::to_string(bytes->count(i) ? bytes->at(i).size() : 0));
        }
        tp.addRow(row);
    }
    tp.print();
    out << " stage ixbar status\n";
    out << ss.str();
    out << "}";
    return out;
}

}  // namespace v2
}  // namespace PHV
