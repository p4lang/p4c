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

#include "bf-p4c/phv/v2/smart_packing.h"

#include <sstream>

#include "bf-p4c/lib/autoclone.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/allocator_base.h"
#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {
namespace v2 {

/// @returns true if stages of writes to a overlapped with @p b's writing stages.
/// When @p n_stages is not zero, they are considered as overlapped if
/// there is a write within @p n_stages stages of a write to the other. This behavior
/// can be overiden if the mau argument is set to non nullptr. In this case, the container
/// conflict evaluation will look at the last table allocation instead.
bool IxbarFriendlyPacking::may_create_container_conflict(const FieldSlice &a, const FieldSlice &b,
                                                         const FieldDefUse &defuse,
                                                         const DependencyGraph &deps,
                                                         const TablesMutuallyExclusive &table_mutex,
                                                         int n_stages, const MauBacktracker *mau) {
    // TODO: change to field-slice level check when we support it in defuse.
    if (a.field() == b.field()) return false;
    for (const auto &a_def : defuse.getAllDefs(a.field()->id)) {
        if (!a_def.first->is<IR::MAU::Table>()) continue;
        const auto *a_t = a_def.first->to<IR::MAU::Table>();
        for (const auto &b_def : defuse.getAllDefs(b.field()->id)) {
            if (!b_def.first->is<IR::MAU::Table>()) continue;
            const auto *b_t = b_def.first->to<IR::MAU::Table>();
            // same table, either in same action(checked elsewhere), or mutex actions.
            if (a_t == b_t) continue;
            // physical dependency
            if (deps.happens_phys_before(a_t, b_t) || deps.happens_phys_before(b_t, a_t)) continue;
            // mutex tables.
            if (table_mutex(a_t, b_t)) continue;
            // use physical stage if available
            if (mau != nullptr) {
                ordered_set<int> sameStage = mau->inSameStage(a_t, b_t);
                if (sameStage.size() == 0) continue;
                LOG6("possible write conflict based on last table allocation: "
                     << a_t->name << ", " << b_t->name << "@" << *sameStage.begin());
            } else {
                const int a_st = deps.min_stage(a_t);
                const int b_st = deps.min_stage(b_t);
                // won't be in a same stage.
                if (std::abs(a_st - b_st) > n_stages) continue;
                LOG6("possible write conflict: " << a_t->name << "@" << a_st << ", " << b_t->name
                                                 << "@" << b_st);
            }
            return true;
        }
    }
    return false;
}

struct FsPacker {
    std::vector<FieldSlice> curr;
    int n_bits = 0;

    void pack(const FieldSlice &fs) {
        curr.push_back(fs);
        n_bits += fs.size();
    }
    bool is_byte_sized() const { return n_bits > 0 && n_bits % 8 == 0; }
    bool is_alignment_compatible(const FieldSlice &fs,
                                 std::optional<int> unmaterialized_field_align,
                                 const AlignedCluster &aligned) const {
        const int field_align_if_packed_here = ((1 << 10) + n_bits - fs.range().lo) % 8;
        // aligned cluster has alignment constraint, and when field is packed, it will also
        // have an alignment constraint, they needs to be the same.
        if (aligned.alignment()) {
            // mod arithmetic to avoid negative numbers when fs.range().lo < n_bits.
            const int field_align_from_cluster =
                ((1 << 10) + *aligned.alignment() - fs.range().lo) % 8;
            if (field_align_if_packed_here != field_align_from_cluster) {
                return false;
            }
        }
        if (unmaterialized_field_align) {
            if (field_align_if_packed_here != unmaterialized_field_align) {
                return false;
            }
        }
        return true;
    }

    /// @returns empty string if cannot be done through pa_byte_pack.
    /// otherwise, return a string of pa_container_size pragma that can be directly
    /// applied on the program.
    cstring to_pa_byte_pack_pragma() const {
        if (curr.empty()) return cstring::empty;
        std::stringstream ss;
        std::string sep = "";
        std::optional<FieldSlice> last;
        ss << "@pa_byte_pack(\"" << curr.front().field()->gress << "\", ";
        for (const auto &fs : curr) {
            if (last && fs.field() == last->field()) {
                last = fs;
                continue;
            }
            if (last) {
                // last was not a complete field.
                if (last->range().hi != last->field()->size - 1) {
                    return cstring::empty;
                }
            } else {
                // new field slice was not starting from 0.
                if (fs.range().lo != 0) {
                    return cstring::empty;
                }
            }
            ss << sep << "\"" << fs.field()->name.findlast(':') + 1 << "\"";
            sep = ", ";
            last = fs;
        }
        if (n_bits % 8 != 0) {
            ss << ", " << 8 - (n_bits % 8);
        }
        ss << ")";
        return ss.str();
    }
};

bool IxbarFriendlyPacking::can_pack(const std::vector<FieldSlice> &slices,
                                    const FieldSlice &fs) const {
    if (fs.field()->is_solitary()) {
        return false;
        LOG3("Found pa_solitary constraint in " << fs);
    }
    // action packing constraints
    for (const auto &packed : slices) {
        if (packed.field()->is_solitary()) {
            return false;
            LOG3("Found pa_solitary constraint in " << packed);
        }
        if (fs.field() != packed.field() && has_pack_conflict_i(packed, fs)) {
            LOG3("Found action packing conflict " << packed << " vs " << fs);
            return false;
        }
    }
    // parser packing
    FieldSliceAllocStartMap fs_starts;
    int offset = 0;
    for (const auto &packed : slices) {
        fs_starts[packed] = offset;
        offset += fs.size();
    }
    fs_starts[fs] = offset;
    auto *err = parser_packing_validator_i->can_pack(fs_starts);
    if (err != nullptr) {
        LOG3("Found parser packing conflict: " << err->str());
        return false;
    }
    // min-stage-based heuristics if mau_i == nullptr
    for (const auto &packed : slices) {
        if (may_create_container_conflict(packed, fs, defuse_i, deps_i, table_mutex_i, 1, mau_i)) {
            LOG3("may_create_container_conflict: " << packed << " v.s. " << fs);
            return false;
        }
    }
    return true;
}

IxbarFriendlyPacking::MergedCluster IxbarFriendlyPacking::merge_by_packing(
    const std::vector<FieldSlice> &packed, const ordered_map<FieldSlice, SuperCluster *> &fs_sc) {
    // create an empty sc and then merge in all slice-related super clusters.
    MergedCluster rst;
    rst.merged = new SuperCluster(ordered_set<const RotationalCluster *>(),
                                  ordered_set<SuperCluster::SliceList *>());
    ordered_set<SuperCluster *> to_be_packed_sc;
    for (auto &fs : packed) {
        LOG5("\tmerge_by_packing fs: " << fs);
        rst.from.insert(fs_sc.at(fs));
        to_be_packed_sc.insert(fs_sc.at(fs));
    }
    for (const auto *sc : to_be_packed_sc) {
        rst.merged = rst.merged->merge(sc);
    }
    ///// update alignment.
    // start with a large number to avoid underflow in computing alignment when update
    // alignment for a field slice, e.g. {f1[1:7], f2<1>}.
    int offset = (1 << 10);
    for (auto &fs : packed) {
        auto *field = phv_i.field(fs.field()->id);
        const int field_alignment = (offset - fs.range().lo) % 8;
        LOG5("\t fs: " << fs << " " << field->name << fs.range()
                       << " alignment orig: " << field->alignment->align
                       << " calc:" << field_alignment << " offset: " << offset);
        rst.original_alignments[field] = field->alignment;
        BUG_CHECK(!field->alignment || int(field->alignment->align) == field_alignment,
                  "incompatible alignment");
        // inject alignments for these fieldslice
        field->updateAlignment(PHV::AlignmentReason::PA_BYTE_PACK,
                               FieldAlignment(le_bitrange(StartLen(field_alignment, field->size))),
                               Util::SourceInfo());
        offset += fs.size();
    }

    // refresh all fieldslice of this field in this supercluster.
    ordered_set<FieldSlice> to_be_packed_fs;
    for (const auto &fs : packed) to_be_packed_fs.insert(fs);
    ordered_set<PHV::SuperCluster::SliceList *> refreshed_merged_sl;
    for (auto sl : rst.merged->slice_lists()) {
        auto refreshed_sl = new PHV::SuperCluster::SliceList();
        for (auto fs : *sl) {
            if (to_be_packed_fs.count(fs)) continue;
            refreshed_sl->push_back(PHV::FieldSlice(fs.field(), fs.range()));
        }
        if (!refreshed_sl->empty()) refreshed_merged_sl.insert(refreshed_sl);
    }

    // inject the extra slice list to pack these candidates.
    auto sl = new PHV::SuperCluster::SliceList();
    for (const auto &fs : packed) {
        sl->push_back(PHV::FieldSlice(fs.field(), fs.range()));
    }
    refreshed_merged_sl.insert(sl);

    // create the refreshed rotational clusters.
    ordered_set<const PHV::RotationalCluster *> refreshed_merged_clusters;
    for (auto roc : rst.merged->clusters()) {
        ordered_set<PHV::AlignedCluster *> refreshed_clusters;
        for (auto cluster : roc->clusters()) {
            ordered_set<PHV::FieldSlice> refreshed_slices;
            for (auto slice : cluster->slices()) {
                refreshed_slices.insert(PHV::FieldSlice(slice.field(), slice.range()));
            }
            refreshed_clusters.insert(new PHV::AlignedCluster(cluster->kind(), refreshed_slices));
        }
        refreshed_merged_clusters.insert(new PHV::RotationalCluster(refreshed_clusters));
    }

    // finalize the merged super cluster.
    rst.merged = new PHV::SuperCluster(refreshed_merged_clusters, refreshed_merged_sl);
    return rst;
}

std::vector<std::pair<const IR::MAU::Table *, std::vector<FieldSlice>>>
IxbarFriendlyPacking::make_table_key_candidates(const std::list<SuperCluster *> &clusters) const {
    ordered_map<const Field *, ordered_set<FieldSlice>> f_fs;
    ordered_map<FieldSlice, SuperCluster::SliceList *> fs_sl;
    ordered_set<FieldSlice> constrained_by_aligned_cluster;
    for (const auto &sc : clusters) {
        for (const auto &sl : sc->slice_lists()) {
            for (const auto &fs : *sl) {
                fs_sl[fs] = sl;
            }
        }
        sc->forall_fieldslices([&](const FieldSlice &fs) {
            f_fs[fs.field()].insert(fs);
            auto aligned_cluster = sc->aligned_cluster(fs);
            // collect all fieldslices that are in AlignedCluster.
            if (aligned_cluster.slices().size() > 1) constrained_by_aligned_cluster.insert(fs);
        });
    }

    const auto get_fined_sliced_fs = [&](const FieldSlice &fs) {
        ordered_set<FieldSlice> rst;
        for (const auto &fine_sliced_fs : f_fs.at(fs.field())) {
            if (fs.range().contains(fine_sliced_fs.range())) {
                rst.insert(fine_sliced_fs);
            } else {
                BUG_CHECK(!fine_sliced_fs.range().overlaps(fs.range()), "not fine sliced");
            }
        }
        return rst;
    };

    ordered_map<const IR::MAU::Table *, std::vector<FieldSlice>> to_be_packed;
    for (const auto &table_prop : tables_i.props()) {
        auto *tb = table_prop.first;
        const auto &prop = table_prop.second;
        ordered_set<FieldSlice> slices;
        // indexes of fields when used as key.
        ordered_map<const Field *, int> field_indexes;
        for (const auto &key : prop.keys) {
            const auto fine_sliced_key = get_fined_sliced_fs(key);
            for (const auto &fs : fine_sliced_key) {
                // do not support pack field slices that were already in a slice list.
                if (fs_sl.count(fs)) continue;
                // packing byte-sized slices will not help us.
                if (fs.size() % 8 == 0) continue;
                // skip field with alignment for now: they will be in a slice list..
                if (fs.field()->alignment) continue;
                // skip fieldslices that are in AlignedCluster, since these fieldslices have
                // implicit alignment constraints.
                if (constrained_by_aligned_cluster.count(fs)) continue;
                slices.insert(fs);
                if (!field_indexes.count(fs.field())) {
                    field_indexes[fs.field()] = field_indexes.size();
                }
            }
        }
        if (slices.size() > 1) {
            to_be_packed[tb] = {slices.begin(), slices.end()};
            sort(to_be_packed[tb].begin(), to_be_packed[tb].end(),
                 [&](const FieldSlice &a, const FieldSlice &b) {
                     if (a.field() != b.field()) {
                         // not field->id because it is not user friendly enough if phv
                         // fitting result changed because the order of field changed.
                         // return a.field()->id < b.field()->id;
                         return field_indexes.at(a.field()) < field_indexes.at(b.field());
                     }
                     return a.range().lo < b.range().lo;
                 });
        }
    }

    // TODO:
    // we only support whole field packing for now. Even if we can pack
    // a slice of the field, because we need to update alignment of the whole field,
    // we need to run AllocVerifier on all related super clusters. It is possible
    // but not that urgent for now. So we limit scope to pack the whole field only.
    // filter out non-whole-field slices.
    {
        ordered_map<const IR::MAU::Table *, std::vector<FieldSlice>> whole_field_to_be_packed;
        for (auto &kv : to_be_packed) {
            const auto *tb = kv.first;
            const auto &slices = kv.second;
            ordered_map<const Field *, bitvec> not_covered;
            for (auto &fs : slices) {
                not_covered[fs.field()].setrange(0, fs.field()->size);
            }
            for (auto &fs : slices) {
                not_covered[fs.field()].clrrange(fs.range().lo, fs.range().size());
            }
            // filter out non-whole-field slices.
            for (const auto &fs : slices) {
                if (not_covered[fs.field()].empty()) {
                    whole_field_to_be_packed[tb].push_back(fs);
                }
            }
        }
        to_be_packed = whole_field_to_be_packed;
    }

    std::vector<const IR::MAU::Table *> sorted;
    for (const auto &tb : Keys(to_be_packed)) sorted.push_back(tb);
    std::sort(sorted.begin(), sorted.end(),
              [&](const IR::MAU::Table *t1, const IR::MAU::Table *t2) {
                  const auto &a = tables_i.prop(t1);
                  const auto &b = tables_i.prop(t2);
                  if (a.is_tcam != b.is_tcam) return a.is_tcam > b.is_tcam;
                  if (a.n_entries != b.n_entries) return a.n_entries > b.n_entries;
                  return false;
              });

    std::vector<std::pair<const IR::MAU::Table *, std::vector<FieldSlice>>> rst;
    for (const auto *tb : sorted) {
        rst.push_back({tb, to_be_packed.at(tb)});
    }
    return rst;
}

std::list<SuperCluster *> IxbarFriendlyPacking::pack(const std::list<SuperCluster *> &clusters) {
    const auto to_be_packed = make_table_key_candidates(clusters);
    Log::TempIndent indent;
    LOG3("Sorted smarting packing table order and slices:" << indent);
    for (const auto &tb_slices : to_be_packed) {
        LOG3("For Table :" << tb_slices.first->name);
        LOG3("Slices: " << tb_slices.second);
    }

    alloc_metrics.start_clock();

    // list of clusters that will be returned in the end.
    ordered_set<SuperCluster *> updated_clusters;
    for (const auto &sc : clusters) {
        updated_clusters.insert(sc);
    }
    const auto get_fs_sc_map = [&updated_clusters]() {
        ordered_map<FieldSlice, SuperCluster *> fs_sc;
        for (const auto &sc : updated_clusters) {
            sc->forall_fieldslices([&](const FieldSlice &fs) { fs_sc[fs] = sc; });
        }
        return fs_sc;
    };
    ordered_set<FieldSlice> packed;
    for (const auto &tb_slices : to_be_packed) {
        const auto *tb = tb_slices.first;
        const auto &slices = tb_slices.second;
        LOG3("Construct packing for table: " << tb->name);
        // fieldslice may not have been packed yet: it is in the list by itself.
        ordered_map<const Field *, std::optional<int>> unmaterialized_alignments;
        std::vector<FsPacker> workers;
        for (const auto &fs : slices) {
            if (packed.count(fs)) {
                LOG3("\tSkip already packed: " << fs);
                continue;
            }
            LOG3("\tTry to pack: " << fs);
            int packer_idx = workers.size();
            const auto &fs_sc = get_fs_sc_map();
            for (size_t i = 0; i < workers.size(); i++) {
                if (workers[i].is_byte_sized()) continue;
                if (!workers[i].is_alignment_compatible(fs, unmaterialized_alignments[fs.field()],
                                                        fs_sc.at(fs)->aligned_cluster(fs))) {
                    LOG3("\t\tincompatible alignment, cannot pack with worker " << i);
                    continue;
                }
                if (!can_pack(workers[i].curr, fs)) continue;
                LOG3("\t\tTry allocation with worker: " << i);
                /// verify by trying to allocate the proposed super cluster.
                std::vector<FieldSlice> packing = workers[i].curr;
                packing.push_back(fs);
                auto merge_rst = merge_by_packing(packing, fs_sc);
                const bool can_be_allocated = can_alloc_i(merge_rst.merged, alloc_metrics);
                if (can_be_allocated) {
                    LOG3("\t\tAllocation is OK. We will merge:");
                    for (auto *to_remove : merge_rst.from) {
                        LOG3(to_remove);
                        updated_clusters.erase(to_remove);
                    }
                    LOG3("\t\t====> into " << merge_rst.merged);
                    updated_clusters.insert(merge_rst.merged);
                    packer_idx = i;
                    break;
                } else {
                    LOG3("\t\tCannot be allocated, try next worker");
                    // restore alignment constraints on fields.
                    for (const auto &kv : merge_rst.original_alignments) {
                        auto *f = phv_i.field(kv.first->id);
                        f->eraseAlignment();
                        if (kv.second) {
                            f->updateAlignment(PHV::AlignmentReason::PA_BYTE_PACK, *kv.second,
                                               Util::SourceInfo());
                        }
                    }
                }
            }
            if (packer_idx == int(workers.size())) {
                auto new_packer = FsPacker();
                if (new_packer.is_alignment_compatible(fs, unmaterialized_alignments[fs.field()],
                                                       fs_sc.at(fs)->aligned_cluster(fs))) {
                    LOG3("\t\tPack into a new worker: " << packer_idx);
                    workers.push_back(FsPacker());
                    unmaterialized_alignments[fs.field()] = ((1 << 10) - fs.range().lo) % 8;
                } else {
                    LOG3(
                        "\t\tskip this slice because its alignment is"
                        " incompatible with an empty list "
                        << fs);
                    continue;
                }
            }
            workers[packer_idx].pack(fs);
        }
        for (const auto &pack : workers) {
            if (pack.curr.size() > 1) {
                // we found a packing!
                for (const auto &fs : pack.curr) {
                    packed.insert(fs);
                }
                // refresh alignments and add to result.
                auto merge_rst = merge_by_packing(pack.curr, get_fs_sc_map());
                BUG_CHECK(merge_rst.from.size() == 1, "not merged yet?");
                updated_clusters.erase(merge_rst.from.front());
                updated_clusters.insert(merge_rst.merged);
                LOG1("\t\tDecided packing: " << pack.curr);
                LOG1("\t\tDecided packing in pragma: " << pack.to_pa_byte_pack_pragma());
                LOG1("\t\tPacked cluster: " << merge_rst.merged);
            }
        }
    }

    alloc_metrics.stop_clock();
    LOG1(alloc_metrics);

    std::list<SuperCluster *> after_merge(updated_clusters.begin(), updated_clusters.end());
    return after_merge;
}

}  // namespace v2
}  // namespace PHV
