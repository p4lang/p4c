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

#include "bf-p4c/phv/v2/phv_kit.h"

#include "bf-p4c/phv/slicing/phv_slicing_iterator.h"

namespace PHV {
namespace v2 {

namespace {

void merge_slices(safe_vector<PHV::AllocSlice>& slices,
                  safe_vector<PHV::AllocSlice>& merged_alloc) {
    std::optional<PHV::AllocSlice> last = std::nullopt;
    for (auto& slice : slices) {
        if (last == std::nullopt) {
            last = slice;
            continue;
        }
        if (last->container() == slice.container() &&
            last->field_slice().lo == slice.field_slice().hi + 1 &&
            last->container_slice().lo == slice.container_slice().hi + 1 &&
            last->getEarliestLiveness() == slice.getEarliestLiveness() &&
            last->getLatestLiveness() == slice.getLatestLiveness()) {
            int new_width = last->width() + slice.width();
            PHV::AllocSlice new_slice(slice.field(), slice.container(), slice.field_slice().lo,
                                      slice.container_slice().lo, new_width, {});
            new_slice.setLiveness(slice.getEarliestLiveness(), slice.getLatestLiveness());
            new_slice.setIsPhysicalStageBased(slice.isPhysicalStageBased());
            BUG_CHECK(new_slice.field_slice().contains(last->field_slice()),
                      "Merged alloc slice %1% does not contain hi slice %2%",
                      cstring::to_cstring(new_slice), cstring::to_cstring(*last));
            BUG_CHECK(new_slice.field_slice().contains(slice.field_slice()),
                      "Merged alloc slice %1% does not contain lo slice %2%",
                      cstring::to_cstring(new_slice), cstring::to_cstring(slice));
            LOG4("Merging " << last->field() << ": " << *last << " and " << slice << " into "
                            << new_slice);
            last = new_slice;
        } else {
            merged_alloc.push_back(*last);
            last = slice;
        }
    }
    if (last) merged_alloc.push_back(*last);
}

}  // namespace

bool PhvKit::has_pack_conflict(const PHV::FieldSlice& fs1, const PHV::FieldSlice& fs2) const {
    if (fs1.field() != fs2.field()) {
        if (mutex()(fs1.field()->id, fs2.field()->id)) return false;
    }
    bool clustering_no_pack = clustering.no_pack(fs1.field(), fs2.field());
    bool action_no_pack = actions.hasPackConflict(fs1, fs2);
    LOG6("has_pack_conflicts: cluster-driven: " << clustering_no_pack <<
         "  action-driven: " << action_no_pack);
    return clustering_no_pack || action_no_pack;
}

PHV::Slicing::IteratorInterface* PhvKit::make_slicing_ctx(const PHV::SuperCluster* sc) const {
    return new PHV::Slicing::ItrContext(phv, field_to_parser_states, parser_info,
                                        sc, pragmas.pa_container_sizes().field_to_layout(),
                                        *packing_validator,
                                        *parser_packing_validator,
                                        boost::bind(&PhvKit::has_pack_conflict, this, _1, _2),
                                        boost::bind(&PhvKit::is_referenced, this, _1));
}

bool PhvKit::can_logical_liverange_be_overlaid(const PHV::AllocSlice& a,
                                               const PHV::AllocSlice& b) const {
    // Determine dependency direction (from / to) based on live range
    bool ABeforeB = a.getEarliestLiveness() < b.getEarliestLiveness();
    const PHV::AllocSlice& fromSlice = ABeforeB ? a : b;
    const PHV::AllocSlice& toSlice   = ABeforeB ? b : a;

    LOG6("From Slice : " << fromSlice << " --> To Slice : " << toSlice);
    std::map<int, const IR::MAU::Table*> id_from_tables;
    std::map<int, const IR::MAU::Table*> id_to_tables;
    for (const auto& from_locpair : defuse.getAllDefsAndUses(fromSlice.field())) {
        const auto from_table = from_locpair.first->to<IR::MAU::Table>();
        if (!from_table) continue;
        id_from_tables[from_table->id] = from_table;
        LOG6("Table Read / Write (From Slice): " << from_table->name);
    }
    for (const auto& to_locpair : defuse.getAllDefsAndUses(toSlice.field())) {
        const auto to_table = to_locpair.first->to<IR::MAU::Table>();
        if (!to_table) continue;
        id_to_tables[to_table->id] = to_table;
        LOG6("Table Read / Write (To Slice): " << to_table->name);
    }
    for (const auto& from_table : id_from_tables) {
        for (const auto& to_table : id_to_tables) {
            if (from_table.second == to_table.second)
                continue;

            auto mutex_tables = table_mutex(from_table.second, to_table.second);
            if (mutex_tables) {
                LOG5("Tables " << from_table.second->name << " and "
                        << to_table.second->name << " are mutually exclusive");
                continue;
            }

            if (deps.happens_logi_after(from_table.second, to_table.second)) {
                LOG5("Table " << from_table.second->name << " have dependencies on " <<
                     to_table.second->name);
                return false;
            }
        }
    }
    return true;
}

bool PhvKit::can_physical_liverange_be_overlaid(const PHV::AllocSlice& a,
                                                const PHV::AllocSlice& b) const {
    BUG_CHECK(a.isPhysicalStageBased() && b.isPhysicalStageBased(),
              "physical liverange overlay should be checked "
              "only when both slices are physical stage based");
    // TODO: need more thoughts on these constraints.
    // (1) is_invalidate_from_arch might be needed only for tofino1.
    const auto never_overlay = [](const PHV::Field* f) {
        return f->pov || f->deparsed_to_tm() || f->is_invalidate_from_arch();
    };
    if (never_overlay(a.field()) || never_overlay(b.field())) {
        return false;
    }
    BUG_CHECK(a.container() == b.container(),
              "checking overlay on different container: %1% and %2%", a.container(), b.container());
    const auto& cont = a.container();
    const bool is_a_deparsed = a.getLatestLiveness().first == Device::numStages();
    const bool is_b_deparsed = b.getLatestLiveness().first == Device::numStages();
    const bool both_deparsed = is_a_deparsed && is_b_deparsed;
    // TODO: The better and less constrainted checks are
    // `may be checksummed together` and `may be deparsed together`,
    // instead of will be both be deparsed. But since it is very likely that
    // the less constrainted checks will not give us any benefits (because
    // we are likely to have enough containers for those read-only fields),
    // we will just check whether they are both deparsed.
    if (both_deparsed) {
        // checksum engine cannot read a container multiple times.
        if (a.field()->is_checksummed() && b.field()->is_checksummed()) {
            return false;
        }
        // deparser cannot emit a non-8-bit container multiple times.
        if (cont.size() != 8) {
            return false;
        }
    }
    if (!pragmas.pa_no_overlay().can_overlay(a.field(), b.field()))
        return false;

    if (a.isLiveRangeDisjoint(b, 1))
        return true;

    if (a.isLiveRangeDisjoint(b, 0)) {
        // We have to make sure that the last use does not have a dependency on the new def.
        LOG5("Slice " << a << " and Slice " << b << " must have its dependencies analyzed");
        return can_logical_liverange_be_overlaid(a, b);
    }
    return false;
}

bool PhvKit::is_clot_allocated(const ClotInfo& clots, const PHV::SuperCluster& sc) {
    // If the device doesn't support CLOTs, then don't bother checking.
    if (Device::numClots() == 0) return false;

    // In JBay, a clot-candidate field may sometimes be allocated to a PHV
    // container, eg. if it is adjacent to a field that must be packed into a
    // larger container, in which case the clot candidate would be used as
    // padding.

    // Check slice lists.
    bool needPhvAllocation = std::any_of(
        sc.slice_lists().begin(), sc.slice_lists().end(),
        [&](const PHV::SuperCluster::SliceList* slices) {
            return std::any_of(slices->begin(), slices->end(), [&](const PHV::FieldSlice& slice) {
                return !clots.fully_allocated(slice);
            });
        });

    // Check rotational clusters.
    needPhvAllocation |= std::any_of(
        sc.clusters().begin(), sc.clusters().end(), [&](const PHV::RotationalCluster* cluster) {
            return std::any_of(
                cluster->slices().begin(), cluster->slices().end(),
                [&](const PHV::FieldSlice& slice) { return !clots.fully_allocated(slice); });
        });

    return !needPhvAllocation;
}

std::list<PHV::SuperCluster*> PhvKit::remove_singleton_metadata_slicelist(
    const std::list<PHV::SuperCluster*>& cluster_groups) {
    std::list<PHV::SuperCluster*> rst;
    for (auto* super_cluster : cluster_groups) {
        // Supercluster has more than one slice list.
        if (super_cluster->slice_lists().size() != 1) {
            rst.push_back(super_cluster);
            continue;
        }
        auto* slice_list = super_cluster->slice_lists().front();
        // The slice list has more than one fieldslice.
        if (slice_list->size() != 1) {
            rst.push_back(super_cluster);
            continue;
        }
        // The fieldslice is pov, or not metadata. Nor does supercluster is singleton.
        // TODO: These comments are vague, I assume that it was trying to find
        // super clusters that have
        // (1) only one slice list
        // (2) only one field in the slice list.
        // (3) only one field in the rotational cluster.
        // (4) no special constraints on the field.
        // and these super clusters does not need to have the field in a slice list because
        // the alignment searched for fields in slice lists is less than fields that are not.
        auto fs = slice_list->front();
        if (fs.size() != int(super_cluster->aggregate_size()) || !fs.field()->metadata ||
            fs.field()->exact_containers() || fs.field()->pov || fs.field()->is_checksummed() ||
            fs.field()->deparsed_bottom_bits() || fs.field()->is_marshaled()) {
            rst.push_back(super_cluster);
            continue;
        }

        ordered_set<PHV::SuperCluster::SliceList*> empty;
        PHV::SuperCluster* new_cluster = new PHV::SuperCluster(super_cluster->clusters(), empty);
        rst.push_back(new_cluster);
    }
    return rst;
}

std::list<PHV::SuperCluster*> PhvKit::remove_unref_clusters(
    const PhvUse& uses, const std::list<PHV::SuperCluster*>& cluster_groups_input) {
    std::list<PHV::SuperCluster*> cluster_groups_filtered;
    for (auto* sc : cluster_groups_input) {
        if (sc->all_of_fieldslices([&](const PHV::FieldSlice& fs) {
                return !uses.is_allocation_required(fs.field());
            })) {
            continue;
        }
        cluster_groups_filtered.push_back(sc);
    }
    return cluster_groups_filtered;
}

std::list<PHV::SuperCluster*> PhvKit::remove_clot_allocated_clusters(
    const ClotInfo& clot, std::list<PHV::SuperCluster*> clusters) {
    auto res = std::remove_if(clusters.begin(), clusters.end(), [&](const PHV::SuperCluster* sc) {
        return PhvKit::is_clot_allocated(clot, *sc);
    });
    clusters.erase(res, clusters.end());
    return clusters;
}

std::list<PHV::ContainerGroup*> PhvKit::make_device_container_groups() {
    const PhvSpec& phvSpec = Device::phvSpec();
    std::list<PHV::ContainerGroup*> rv;

    // Build MAU groups
    for (const PHV::Size s : phvSpec.containerSizes()) {
        for (auto group : phvSpec.mauGroups(s)) {
            // Get type of group
            if (group.empty()) continue;
            // Create group.
            rv.emplace_back(new PHV::ContainerGroup(s, group));
        }
    }

    // Build TPHV collections
    for (auto collection : phvSpec.tagalongCollections()) {
        // Each PHV_MAU_Group holds containers of the same size.  Hence, TPHV
        // collections are split into three groups, by size.
        ordered_map<PHV::Type, bitvec> groups_by_type;

        // Add containers to groups by size
        for (auto cid : collection) {
            auto type = phvSpec.idToContainer(cid).type();
            groups_by_type[type].setbit(cid);
        }

        for (auto kv : groups_by_type)
            rv.emplace_back(new PHV::ContainerGroup(kv.first.size(), kv.second));
    }

    return rv;
}

void PhvKit::clear_slices(PhvInfo& phv) {
    phv.clear_container_to_fields();
    for (auto& f : phv) f.clear_alloc();
}

void PhvKit::bind_slices(const PHV::ConcreteAllocation& alloc, PhvInfo& phv) {
    for (auto container_and_slices : alloc) {
        for (PHV::AllocSlice slice : container_and_slices.second.slices) {
            PHV::Field* f = const_cast<PHV::Field*>(slice.field());
            f->add_alloc(slice);
            phv.add_container_to_field_entry(slice.container(), f);
        }
    }
}

std::list<PHV::SuperCluster*> PhvKit::create_strided_clusters(
    const CollectStridedHeaders& strided_headers,
    const std::list<PHV::SuperCluster*>& cluster_groups) {
    std::list<PHV::SuperCluster*> rst;
    for (auto* sc : cluster_groups) {
        const ordered_set<const PHV::Field*>* strided_group = nullptr;
        auto slices = sc->slices();
        for (const auto& sl : slices) {
            strided_group = strided_headers.get_strided_group(sl.field());
            if (!strided_group) continue;
            if (slices.size() != 1) {
                error(
                    "Field %1% requires strided allocation"
                    " but has conflicting constraints on it.",
                    sl.field()->name);
            }
            break;
        }
        if (!strided_group) {
            rst.push_back(sc);
            continue;
        }
        bool merged = false;
        for (auto* r : rst) {
            auto r_slices = r->slices();
            for (const auto& sl : r_slices) {
                if (!strided_group->count(sl.field())) continue;
                if (r_slices.front().range() == slices.front().range()) {
                    rst.remove(r);
                    auto m = r->merge(sc);
                    m->needsStridedAlloc(true);
                    rst.push_back(m);
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
        if (!merged) rst.push_back(sc);
    }

    return rst;
}

void PhvKit::sort_and_merge_alloc_slices(PhvInfo& phv) {
    // later passes assume that phv alloc info is sorted in field bit order,
    // msb first

    for (auto& f : phv) f.sort_alloc();
    // Merge adjacent field slices that have been allocated adjacently in the
    // same container.  This can happen when the field is involved in a set
    // instruction with another field that has been split---it needs to be
    // "split" to match the invariants on rotational clusters, but in practice
    // to the two slices remain adjacent.
    for (auto& f : phv) {
        safe_vector<PHV::AllocSlice> merged_alloc;
        // In table first phv allocation, field slice live range should be taken into consideration.
        // Therefore, after allocslice is sorted, all alloc slices need to be clustered by live
        // range. And then merge allocslices based on each cluster.
        ordered_map<PHV::LiveRange, safe_vector<PHV::AllocSlice>> liverange_classified;
        if (BFNContext::get().options().alt_phv_alloc == true) {
            for (auto& slice : f.get_alloc()) {
                liverange_classified[{slice.getEarliestLiveness(), slice.getLatestLiveness()}]
                    .push_back(slice);
            }
            for (auto& slices : Values(liverange_classified)) {
                merge_slices(slices, merged_alloc);
            }
        } else {
            merge_slices(f.get_alloc(), merged_alloc);
        }
        f.set_alloc(merged_alloc);
        f.sort_alloc();
    }
}

bool PhvKit::is_ternary(const IR::MAU::Table* tbl) {
    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("ternary"_cs)) {
        if (s->expr.size() <= 0) {
            return false;
        } else {
            auto* pragma_val = s->expr.at(0)->to<IR::Constant>();
            ERROR_CHECK(pragma_val != nullptr, ErrorType::ERR_UNKNOWN,
                        "unknown ternary pragma %1% on table %2%.", s, tbl->externalName());
            if (pragma_val->asInt() == 1) {
                return true;
            } else {
                return false;
            }
        }
    } else {
        for (auto ixbar_read : tbl->match_key) {
            if (ixbar_read->match_type.name == "ternary" || ixbar_read->match_type.name == "lpm") {
                return true;
            }
        }
        for (auto ixbar_read : tbl->match_key) {
            if (ixbar_read->match_type == "range") {
                return true;
            }
        }
    }
    return false;
}

}  // namespace v2
}  // namespace PHV
