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

#include "bf-p4c/phv/utils/utils.h"

#include <deque>
#include <iostream>
#include <numeric>
#include <optional>

#include <boost/optional/optional_io.hpp>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/utils/live_range_report.h"
#include "bf-p4c/phv/utils/report.h"
#include "lib/algorithm.h"

static int cluster_id_g = 0;  // global counter for assigning cluster ids

int PHV::ClusterStats::nextId = 0;

PHV::Allocation::ContainerStatus PHV::ConcreteAllocation::emptyContainerStatus;

PHV::ContainerGroup::ContainerGroup(PHV::Size sz, const std::vector<PHV::Container> containers)
    : size_i(sz), containers_i(containers) {
    // Check that all containers are the right size.
    for (auto c : containers_i) {
        BUG_CHECK(c.type().size() == size_i,
                  "PHV container group constructed with size %1% but has container %2% of size %3%",
                  cstring::to_cstring(size_i), cstring::to_cstring(c),
                  cstring::to_cstring(c.size()));
        types_i.insert(c.type());
        ids_i.setbit(Device::phvSpec().containerToId(c));
    }
}

PHV::ContainerGroup::ContainerGroup(PHV::Size sz, bitvec container_group)
    : size_i(sz), ids_i(container_group) {
    const PhvSpec &phvSpec = Device::phvSpec();
    for (auto cid : container_group) {
        auto c = phvSpec.idToContainer(cid);
        BUG_CHECK(c.type().size() == size_i,
                  "PHV container group constructed with size %1% but has container %2% of size %3%",
                  cstring::to_cstring(size_i), cstring::to_cstring(c),
                  cstring::to_cstring(c.size()));
        types_i.insert(c.type());
        containers_i.push_back(c);
    }
}

bool PHV::Allocation::addStatus(PHV::Container c, const ContainerStatus &status) {
    bool new_slice = !(container_status_i.count(c) &&
                       container_status_i[c].slices.size() == status.slices.size());

    // Update container status.
    container_status_i[c] = status;

    // Update field status.
    for (auto &slice : status.slices) {
        // If the field status already contains this slice, then update it with the updated slice
        // with the latest live range.
        if (field_status_i.count(slice.field())) {
            FieldStatus toBeRemoved;
            for (auto &existingSlice : field_status_i[slice.field()]) {
                if (!slice.representsSameFieldSlice(existingSlice)) continue;
                toBeRemoved.insert(existingSlice);
            }
            for (auto &removalCandidate : toBeRemoved)
                field_status_i[slice.field()].erase(removalCandidate);
        }
        field_status_i[slice.field()].insert(slice);
    }
    return new_slice;
}

static const char *fieldName(const PHV::AllocSlice &slice) {
    if (!slice.field()) return "no_name";
    if (auto *t = slice.field()->name.findlast('.')) return t + 1;
    if (auto *t = slice.field()->name.findlast(':')) return t + 1;
    return slice.field()->name;
}

std::ostream &operator<<(std::ostream &out, const PHV::ActionSet &actions) {
    const char *sep = " ";
    out << "{";
    for (auto *act : actions) {
        out << sep << act->name;
        sep = ", ";
    }
    out << (sep + 1) << "}";
    return out;
}

void PHV::Allocation::addMetaInitPoints(const PHV::AllocSlice &slice, const ActionSet &actions) {
    meta_init_points_i[slice] = actions;
    LOG_FEATURE(fieldName(slice), 5, "Setting init points for " << slice << ": " << actions);
    LOG5("Adding init points for " << slice);
    LOG5("Number of entries in meta_init_points_i: " << meta_init_points_i.size());
    if (meta_init_points_i.size() > 0)
        for (const auto &kv : meta_init_points_i)
            LOG5("  Init points for " << kv.first << " : " << kv.second.size());
    for (const IR::MAU::Action *act : actions) init_writes_i[act].insert(slice.field());
}

void PHV::Allocation::addARAedge(gress_t grs, const IR::MAU::Table *src,
                                 const IR::MAU::Table *dst) const {
    ara_edges[grs][src].insert(dst);

    LOG5("Adding ARA edge from table " << src->name << " to table " << dst->name << " in gress "
                                       << grs);
}

std::string PHV::Allocation::printARAedges() const {
    std::stringstream ss;

    for (auto grs_entry : ara_edges) {
        ss << std::endl << "  ARA EDGES : " << grs_entry.first;

        for (auto src_entry : grs_entry.second) {
            auto *src_tbl = src_entry.first;
            ss << std::endl << src_tbl->name << "  --> ";
            int num_dst = 0;

            for (auto *dst_entry : src_entry.second) {
                ss << ++num_dst << "." << dst_entry->name << " ";
            }
        }
    }
    return ss.str();
}

void PHV::Allocation::addSlice(PHV::Container c, PHV::AllocSlice slice) {
    // Get the current status in container_status_i, or its ancestors, if any.
    const auto *container_status = this->getStatus(c);
    ContainerStatus status = container_status ? *container_status : ContainerStatus();
    status.slices.insert(slice);
    container_status_i[c] = status;

    // Update field status.
    field_status_i[slice.field()].insert(slice);

    // Update the allocation status of the container.
    if (container_status_i[c].alloc_status != PHV::Allocation::ContainerAllocStatus::FULL) {
        PHV::Allocation::ContainerAllocStatus old_status = container_status_i[c].alloc_status;
        bitvec allocated_bits;
        for (const auto &slice : container_status_i[c].slices)
            allocated_bits |= bitvec(slice.container_slice().lo, slice.width());
        if (allocated_bits == bitvec())
            container_status_i[c].alloc_status = PHV::Allocation::ContainerAllocStatus::EMPTY;
        else if (allocated_bits == bitvec(0, c.size()))
            container_status_i[c].alloc_status = PHV::Allocation::ContainerAllocStatus::FULL;
        else
            container_status_i[c].alloc_status = PHV::Allocation::ContainerAllocStatus::PARTIAL;

        BUG_CHECK(
            container_status_i[c].alloc_status != PHV::Allocation::ContainerAllocStatus::EMPTY ||
                (container_status_i[c].alloc_status ==
                     PHV::Allocation::ContainerAllocStatus::EMPTY &&
                 container_status_i[c].alloc_status == old_status),
            "Changing allocation status from FULL or PARTIAL to EMPTY");

        if (old_status != container_status_i[c].alloc_status) {
            --count_by_status_i[c.type().size()][old_status];
            ++count_by_status_i[c.type().size()][container_status_i[c].alloc_status];
        }
    }
}

void PHV::Allocation::addMetadataInitialization(PHV::AllocSlice slice,
                                                LiveRangeShrinkingMap initNodes) {
    // If no initialization required for the slice, return.
    if (!initNodes.count(slice.field()))
        // Initialization is on a different AllocSlice.
        return;

    // If initialization required for the slice, then add it.
    this->addMetaInitPoints(slice, initNodes.at(slice.field()));
}

bool PHV::Allocation::addDarkAllocation(const PHV::AllocSlice &slice) {
    // Need not process non-dark containers.
    if (!slice.container().is(PHV::Kind::dark)) return true;
    bitvec writeRange, readRange;
    auto minStage = slice.getEarliestLiveness();
    auto maxStage = slice.getLatestLiveness();
    int minReadStage = (minStage.second == PHV::FieldUse(PHV::FieldUse::READ))
                           ? minStage.first
                           : (minStage.first + 1);
    int minWriteStage = (minStage.second == PHV::FieldUse(PHV::FieldUse::READ))
                            ? (minStage.first - 1)
                            : minStage.first;
    int maxReadStage = (maxStage.second == PHV::FieldUse(PHV::FieldUse::READ))
                           ? maxStage.first
                           : (maxStage.first + 1);
    int maxWriteStage = (maxStage.second == PHV::FieldUse(PHV::FieldUse::READ))
                            ? (maxStage.first - 1)
                            : maxStage.first;
    for (int i = minReadStage; i <= maxReadStage; i++) {
        if (i < 0) continue;
        readRange.setbit(i);
    }
    for (int i = minWriteStage; i <= maxWriteStage; i++) {
        if (i < 0) continue;
        writeRange.setbit(i);
    }
    if (dark_containers_read_allocated_i.count(slice.container())) {
        bitvec combo = dark_containers_read_allocated_i.at(slice.container()) & readRange;
        if (combo.popcount() != 0) {
            LOG3("\t\tThis allocation already overlaps with an existing read range for "
                 << slice.container());
            return false;
        } else {
            dark_containers_read_allocated_i[slice.container()] |= readRange;
        }
    } else {
        dark_containers_read_allocated_i[slice.container()] = readRange;
    }
    if (dark_containers_write_allocated_i.count(slice.container())) {
        bitvec combo = dark_containers_write_allocated_i.at(slice.container()) & writeRange;
        if (combo.popcount() != 0) {
            LOG3("\t\tThis allocation already overlaps with an existing write range for "
                 << slice.container());
            return false;
        } else {
            dark_containers_write_allocated_i[slice.container()] |= writeRange;
        }
    } else {
        dark_containers_write_allocated_i[slice.container()] = writeRange;
    }

    LOG3("\t\t" << slice.container() << " read allocated to slice " << slice << " in stages "
                << minReadStage << "-" << maxReadStage);
    LOG3("\t\t" << slice.container() << " write allocated to slice " << slice << " in stages "
                << minWriteStage << "-" << maxWriteStage);
    return true;
}

void PHV::Allocation::setGress(PHV::Container c, GressAssignment gress) {
    // Get the current status in container_status_i, or its ancestors, if any.
    const auto *container_status = this->getStatus(c);
    ContainerStatus status = container_status ? *container_status : ContainerStatus();
    status.gress = gress;
    container_status_i[c] = status;
}

void PHV::Allocation::setParserGroupGress(PHV::Container c, GressAssignment parserGroupGress) {
    // Get the current status in container_status_i, or its ancestors, if any.
    const auto *container_status = this->getStatus(c);
    ContainerStatus status = container_status ? *container_status : ContainerStatus();
    status.parserGroupGress = parserGroupGress;
    container_status_i[c] = status;
}

void PHV::Allocation::setDeparserGroupGress(PHV::Container c, GressAssignment deparserGroupGress) {
    // Get the current status in container_status_i, or its ancestors, if any.
    const auto *container_status = this->getStatus(c);
    ContainerStatus status = container_status ? *container_status : ContainerStatus();
    status.deparserGroupGress = deparserGroupGress;
    container_status_i[c] = status;
}

void PHV::Allocation::setParserExtractGroupSource(PHV::Container c, ExtractSource source) {
    // Get the current status in container_status_i, or its ancestors, if any.
    const auto *container_status = this->getStatus(c);
    ContainerStatus status = container_status ? *container_status : ContainerStatus();
    status.parserExtractGroupSource = source;
    container_status_i[c] = status;
}

PHV::Allocation::MutuallyLiveSlices PHV::Allocation::liverange_overlapped_slices(
    const PHV::Container c, const std::vector<AllocSlice> &slices) const {
    PHV::Allocation::MutuallyLiveSlices rs;
    foreach_slice(c, [&](const AllocSlice &allocated) {
        if (std::any_of(slices.begin(), slices.end(), [&](const AllocSlice &sl) {
                // not mutex and have overlapped live range.
                return !phv_i->field_mutex()(allocated.field()->id, sl.field()->id) &&
                       !allocated.isLiveRangeDisjoint(sl);
            })) {
            rs.insert(allocated);
        }
    });
    return rs;
}

PHV::Allocation::MutuallyLiveSlices PHV::Allocation::slicesByLiveness(const PHV::Container c,
                                                                      const AllocSlice &sl) const {
    PHV::Allocation::MutuallyLiveSlices rs;
    this->foreach_slice(c, [&](const PHV::AllocSlice &slice) {
        bool mutex = phv_i->field_mutex()(slice.field()->id, sl.field()->id);
        // Checking disjoint liveranges may be too conservative due to
        // default [parser, deparser] liveranges
        bool liverange_mutex = slice.isLiveRangeDisjoint(sl);
        if (!mutex && !liverange_mutex) rs.insert(slice);
    });
    return rs;
}

// This is the same as slicesByLiveness with the addition that it
// handles the liverange overlap from implicit parser initialization
// Note: Implicit parser initialization is not reflected in the slice liverange.
PHV::Allocation::MutuallyLiveSlices PHV::Allocation::byteSlicesByLiveness(
    const PHV::Container c, const AllocSlice &sl, const PragmaNoInit &noInit) const {
    PHV::Allocation::MutuallyLiveSlices rs;
    bool sl_is_extracted = uses_i->is_extracted_from_pkt(sl.field());
    this->foreach_slice(c, [&](const AllocSlice &slice) {
        bool mutex = phv_i->field_mutex()(slice.field()->id, sl.field()->id);
        // *ALEX* Checking disjoint liveranges may be too conservative due to
        // default [parser, deparser] liveranges - See P4C-4467
        bool liverange_mutex = slice.isLiveRangeDisjoint(sl);
        // In TF2/3 extraction can be done at byte granularity compared
        // to container-granularity in TF1.
        bool same_bytes = (Device::currentDevice() == Device::TOFINO) ||
                          sl.container_bytes().overlaps(slice.container_bytes());
        bool extr_in_uninit_byte = false;

        // When slices share bytes check for extraction on non-initialized metadata slice bytes
        // (useful when container slices do not overlap)
        if (same_bytes) {
            // Check if metadata is uninitialized and not marked as pa_no_init
            extr_in_uninit_byte = (sl_is_extracted && !(slice.is_initialized() ||
                                                        noInit.getFields().count(slice.field()))) ||
                                  (uses_i->is_extracted_from_pkt(slice.field()) &&
                                   !(sl.is_initialized() || noInit.getFields().count(sl.field())));
        }

        if (!mutex && (!liverange_mutex || extr_in_uninit_byte)) rs.insert(slice);
    });
    return rs;
}

PHV::Allocation::MutuallyLiveSlices PHV::Allocation::slicesByLiveness(
    const PHV::Container c, std::vector<AllocSlice> &slices) const {
    PHV::Allocation::MutuallyLiveSlices rs;
    // TODO: discrepancy with ^ slicesByLiveness function
    // liveRangeDisjoint not check in this function.
    this->foreach_slice(c, [&](const AllocSlice &slice) {
        if (std::any_of(slices.begin(), slices.end(), [&](const AllocSlice &sl) {
                return !phv_i->field_mutex()(slice.field()->id, sl.field()->id);
            })) {
            rs.insert(slice);
        }
    });
    return rs;
}

std::vector<PHV::Allocation::MutuallyLiveSlices> PHV::Allocation::slicesByLiveness(
    PHV::Container c) const {
    // Idea: set of bitvecs, where each bitvec is a collection of mutually live
    // field IDs.
    auto slices = this->slices(c);
    ordered_map<unsigned, ordered_set<bitvec>> by_round;
    ordered_map<unsigned, ordered_set<bitvec>> remove_by_round;
    ordered_map<int, const PHV::AllocSlice *> fid_to_slice;

    // Populate round 1.
    for (auto &slice : slices) {
        by_round[1U].insert(bitvec(slice.field()->id, 1));
        fid_to_slice[slice.field()->id] = &slice;
    }

    // Start at round 2.
    for (unsigned round = 2U; round <= slices.size(); round++) {
        // Try adding each slice to each set in the previous round.
        for (auto &slice : slices) {
            int fid = slice.field()->id;
            for (bitvec bv : by_round[round - 1]) {
                if (bv[fid]) continue;
                // If fid is live with all fields in bv, add it to bv and
                // schedule bv for eventual removal.
                bool all_live = true;
                for (int fid2 : bv) {
                    if (phv_i->field_mutex()(fid_to_slice.at(fid)->field()->id,
                                             fid_to_slice.at(fid2)->field()->id)) {
                        all_live = false;
                        break;
                    }
                }
                if (all_live) {
                    remove_by_round[round - 1].insert(bv);
                    bv.setbit(fid);
                    by_round[round].insert(bv);
                }
            }
        }
    }

    // Build a set from the non-removed bitvecs of each round.
    ordered_set<bitvec> rvset;
    for (unsigned round = 1; round <= slices.size(); round++) {
        rvset |= by_round[round];
        rvset -= remove_by_round[round];
    }

    std::vector<PHV::Allocation::MutuallyLiveSlices> rv;
    for (bitvec bv : rvset) {
        if (bv.empty()) continue;
        rv.push_back({});
        for (int fid : bv) rv.back().insert(*fid_to_slice[fid]);
    }

    return rv;
}

int PHV::Allocation::empty_containers(PHV::Size size) const {
    auto status = PHV::Allocation::ContainerAllocStatus::EMPTY;
    if (count_by_status_i.find(size) == count_by_status_i.end()) return 0;
    if (count_by_status_i.at(size).find(status) == count_by_status_i.at(size).end()) return 0;
    return count_by_status_i.at(size).at(status);
}

/** Assign @p slice to @p slice.container, updating the gress information of
 * the container and its MAU group if necessary.  Fails if the gress of
 * @p slice.field does not match any gress in the MAU group.
 */
void PHV::Allocation::allocate(PHV::AllocSlice slice, LiveRangeShrinkingMap *initNodes,
                               bool singleGressParserGroup) {
    auto &phvSpec = Device::phvSpec();
    unsigned slice_cid = phvSpec.containerToId(slice.container());
    auto containerGress = this->gress(slice.container());
    auto parserGroupGress = this->parserGroupGress(slice.container());
    auto deparserGroupGress = this->deparserGroupGress(slice.container());
    auto parserExtractGroupSource = this->parserExtractGroupSource(slice.container());
    bool isDeparsed = uses_i->is_deparsed(slice.field());

    // If the container has been pinned to a gress, check that the gress
    // matches that of the slice.  Otherwise, pin it.
    if (containerGress) {
        BUG_CHECK(isTrivial || *containerGress == slice.field()->gress,
                  "Trying to allocate field %1% with gress %2% to container %3% with gress %4%",
                  slice.field()->name, slice.field()->gress, slice.container(),
                  this->gress(slice.container()));
    } else {
        this->setGress(slice.container(), slice.field()->gress);
    }

    // If the slice is extracted, check (and maybe set) the parser group gress.
    if (uses_i->is_extracted(slice.field()) ||
        (uses_i->is_used_mau(slice.field()) && singleGressParserGroup)) {
        if (parserGroupGress) {
            BUG_CHECK(isTrivial || *parserGroupGress == slice.field()->gress,
                      "Trying to allocate field %1% with gress %2% to container %3% with "
                      "parser group gress %4%",
                      slice.field()->name, slice.field()->gress, slice.container(),
                      *parserGroupGress);
        } else {
            for (unsigned cid : phvSpec.parserGroup(slice_cid)) {
                auto c = phvSpec.idToContainer(cid);
                auto cGress = this->parserGroupGress(c);
                BUG_CHECK(isTrivial || !cGress || *cGress == slice.field()->gress,
                          "Container %1% already has parser group gress set to %2%", c,
                          *this->parserGroupGress(c));
                this->setParserGroupGress(c, slice.field()->gress);
                LOG4("\t\t\t ... setting container " << c << " gress to " << slice.field()->gress);
            }
        }
    }

    // If the slice is extracted, check (and maybe set) the parser extract group source
    if (phvSpec.hasParserExtractGroups() && uses_i->is_extracted(slice.field())) {
        bool pktExt = uses_i->is_extracted_from_pkt(slice.field());
        if (parserExtractGroupSource != ExtractSource::NONE) {
            BUG_CHECK((parserExtractGroupSource == ExtractSource::PACKET && pktExt) ||
                          (parserExtractGroupSource == ExtractSource::NON_PACKET && !pktExt),
                      "Trying to allocate field %1% with %2% source to container %3% with "
                      "%4% source",
                      slice.field()->name, pktExt ? "packet" : "non-packet", slice.container(),
                      parserExtractGroupSource);
        } else {
            ExtractSource source = pktExt ? ExtractSource::PACKET : ExtractSource::NON_PACKET;
            for (unsigned cid : phvSpec.parserExtractGroup(slice_cid)) {
                auto c = phvSpec.idToContainer(cid);
                auto cSource = this->parserExtractGroupSource(c);
                BUG_CHECK(cSource == ExtractSource::NONE || cSource == source,
                          "Container %1% already has parser extract group source set to %2%", c,
                          cSource);
                this->setParserExtractGroupSource(c, source);
                LOG4("\t\t\t ... setting container " << c << " source to " << source);
            }
        }
    }

    // If the slice is deparsed but the deparser group gress has not yet been
    // set, then set it for each container in the deparser group.
    if (isDeparsed && !deparserGroupGress) {
        for (unsigned cid : phvSpec.deparserGroup(slice_cid)) {
            auto c = phvSpec.idToContainer(cid);
            auto cGress = this->deparserGroupGress(c);
            BUG_CHECK(!cGress || *cGress == slice.field()->gress,
                      "Container %1% already has deparser group gress set to %2%", c,
                      *this->deparserGroupGress(c));
            this->setDeparserGroupGress(c, slice.field()->gress);
        }
    } else if (isDeparsed) {
        // Otherwise, check that the slice gress (which is equal to the
        // container's gress at this point) matches the deparser group gress.
        BUG_CHECK(isTrivial || slice.field()->gress == *deparserGroupGress,
                  "Cannot allocate %1%, because container is already assigned to %2% but has a "
                  "deparser group assigned to %3%",
                  slice, slice.field()->gress, *deparserGroupGress);
    }

    // Set gress for all the containers in a tagalong group (for container only).
    if (slice.container().is(PHV::Kind::tagalong)) {
        for (unsigned cid : phvSpec.tagalongCollection(slice_cid)) {
            auto c = phvSpec.idToContainer(cid);
            auto cGress = this->gress(c);
            BUG_CHECK(!cGress || *cGress == slice.field()->gress,
                      "Container %1% already has gress set to %2%", c, *cGress);
            this->setGress(c, slice.field()->gress);
        }
    }

    // Update allocation.
    this->addSlice(slice.container(), slice);

    // Remember the initialization points for metadata.
    if (initNodes) this->addMetadataInitialization(slice, *initNodes);
}

void PHV::Allocation::removeAllocatedSlice(const ordered_set<PHV::AllocSlice> &slices) {
    PHV::Container c = (*(slices.begin())).container();
    const auto *container_status = this->getStatus(c);
    ContainerStatus status = container_status ? *container_status : ContainerStatus();
    ordered_set<AllocSlice> toBeRemoved;
    for (auto &sl : status.slices) {
        for (auto &slice : slices) {
            if (!sl.representsSameFieldSlice(slice)) continue;
            toBeRemoved.insert(sl);
            LOG1("\t\t\tNeed to remove " << sl);
        }
    }
    for (auto &sl : toBeRemoved) status.slices.erase(sl);
    container_status_i[c] = status;
    LOG1("\t\t\tNew state of container (after removal)");
    for (auto &sl : this->slices(c)) LOG1("\t\t\t" << sl);
    LOG1("\t\t\tTrying to remove field slices slices");
    for (auto &sl : toBeRemoved) {
        LOG1("\t\t\tTrying to remove field slice: " << sl);
        if (!field_status_i.count(sl.field())) {
            LOG1("\t\t\t  No slices found corresponding to field " << sl.field()->name);
            continue;
        }
        field_status_i[sl.field()].erase(sl);
    }
}

cstring PHV::Allocation::commit(Transaction &view) {
    BUG_CHECK(view.getParent() == this,
              "Trying to commit PHV allocation transaction to an "
              "allocation that is not its parent");

    std::stringstream ss;
    state_to_containers_i.clear();

    // Merge the status from the view.
    for (const auto &kv : view.getTransactionStatus()) {
        bool c_status_chng = this->addStatus(kv.first, kv.second);
        if (c_status_chng) {
            ss << "   " << kv.first << " " << kv.second.parserGroupGress << " "
               << kv.second.deparserGroupGress << "\n";
        }
    }

    for (const auto &map_entry : view.getARAedges()) {
        gress_t grs = map_entry.first;

        for (const auto &src2dsts : map_entry.second) {
            auto *src_tbl = src2dsts.first;

            for (auto *dst_tbl : src2dsts.second) {
                addARAedge(grs, src_tbl, dst_tbl);
            }
        }
    }

    // Merge the metadata initialization points from the view.
    for (const auto &kv : view.getMetaInitPoints()) this->addMetaInitPoints(kv.first, kv.second);

    // Print the initialization information for this transaction.
    if (view.getInitWrites().size() != 0)
        for (const auto &kv : view.getInitWrites())
            this->init_writes_i[kv.first].insert(kv.second.begin(), kv.second.end());

    // Clear the view.
    view.clearTransactionStatus();
    return cstring::to_cstring(ss.str());
}

PHV::Transaction *PHV::Allocation::clone(const Allocation &parent) const {
    PHV::Transaction *rv = new PHV::Transaction(parent);

    for (const auto &kv : container_status_i) {
        bool new_slice = false;
        const auto *parent_status = parent.getStatus(kv.first);
        BUG_CHECK(parent_status,
                  "Trying to get allocation status for container %1% not in Allocation",
                  cstring::to_cstring(kv.first));

        // Only clone the difference between the parent and the child
        for (const AllocSlice &slice : kv.second.slices) {
            if (!parent_status->slices.count(slice)) {
                new_slice = true;
                break;
            }
        }
        if (!new_slice) {
            for (const AllocSlice &slice : parent_status->slices) {
                if (!kv.second.slices.count(slice)) {
                    new_slice = true;
                    break;
                }
            }
        }
        bool gress_assign = !(parent_status->gress == kv.second.gress &&
                              parent_status->parserGroupGress == kv.second.parserGroupGress &&
                              parent_status->deparserGroupGress == kv.second.deparserGroupGress);
        if (!new_slice && !gress_assign) continue;

        rv->container_status_i[kv.first] = kv.second;
    }

    for (const auto &kv : meta_init_points_i)
        rv->meta_init_points_i[kv.first].insert(kv.second.begin(), kv.second.end());

    for (const auto &kv : init_writes_i)
        rv->init_writes_i[kv.first].insert(kv.second.begin(), kv.second.end());

    for (const auto &map_entry : parent.getARAedges()) {
        auto grs = map_entry.first;

        for (const auto &src2dsts : map_entry.second) {
            auto *src_tbl = src2dsts.first;

            for (auto *dst_tbl : src2dsts.second) {
                rv->addARAedge(grs, src_tbl, dst_tbl);
            }
        }
    }

    return rv;
}

PHV::Transaction PHV::Allocation::makeTransaction() const { return Transaction(*this); }

/// @returns a pretty-printed representation of this Allocation.
cstring PHV::Allocation::toString() const { return cstring::to_cstring(*this); }

const ordered_set<const PHV::Field *> PHV::Allocation::getMetadataInits(
    const IR::MAU::Action *act) const {
    ordered_set<const PHV::Field *> emptySet;
    if (!init_writes_i.count(act)) return emptySet;
    return init_writes_i.at(act);
}

PHV::ActionSet PHV::Allocation::getInitPointsForField(const PHV::Field *f) const {
    PHV::ActionSet rs;
    for (const auto &kv : meta_init_points_i) {
        if (kv.first.field() != f) continue;
        rs.insert(kv.second.begin(), kv.second.end());
    }
    return rs;
}

const ordered_set<unsigned> PHV::Allocation::getTagalongCollectionsUsed() const {
    ordered_set<unsigned> rv;

    for (const auto &kv : container_status_i) {
        const auto &container = kv.first;
        const auto &status = kv.second;

        const auto &kind = container.type().kind();
        if (kind != PHV::Kind::tagalong) continue;
        if (status.alloc_status == ContainerAllocStatus::EMPTY) continue;

        unsigned collectionID = Device::phvSpec().getTagalongCollectionId(container);
        rv.insert(collectionID);
    }

    return rv;
}

const ordered_map<const IR::BFN::ParserState *, std::set<PHV::Container>> &
PHV::Allocation::getParserStateToContainers(
    const PhvInfo &, const MapFieldToParserStates &field_to_parser_states) const {
    if (state_to_containers_i.empty()) {
        for (const auto &kv : field_status_i) {
            const auto &field = kv.first;
            const auto &container_slices = kv.second;

            for (const auto &slice : container_slices) {
                if (field_to_parser_states.field_to_parser_states.count(field)) {
                    for (auto state : field_to_parser_states.field_to_parser_states.at(field))
                        state_to_containers_i[state].insert(slice.container());
                }
            }
        }
    }

    return state_to_containers_i;
}

std::set<PHV::Allocation::AvailableSpot> PHV::Allocation::available_spots() const {
    std::set<AvailableSpot> rst;
    // Compute status.
    for (auto cid : Device::phvSpec().physicalContainers()) {
        PHV::Container c = Device::phvSpec().idToContainer(cid);
        auto slices = this->slices(c);
        auto gress = this->gress(c);
        auto parserGroupGress = this->parserGroupGress(c);
        auto deparserGroupGress = this->deparserGroupGress(c);
        auto parserExtractGroupSource = this->parserExtractGroupSource(c);
        // Empty
        if (slices.size() == 0) {
            rst.insert(AvailableSpot(c, gress, parserGroupGress, deparserGroupGress,
                                     parserExtractGroupSource, c.size()));
            continue;
        }
        // calculate allocate bitvec
        bitvec allocatedBits;
        for (const auto &slice : slices) {
            allocatedBits |= bitvec(slice.container_slice().lo, slice.container_slice().size());
        }
        // Full
        if (allocatedBits == bitvec(0, c.size())) {
            continue;
        } else {
            // Occupied by solitary field
            if (slices.size() == 1 && slices.begin()->field()->is_solitary()) {
                continue;
            }
            int used = std::accumulate(allocatedBits.begin(), allocatedBits.end(), 0,
                                       [](int a, int) { return a + 1; });
            rst.insert(AvailableSpot(c, gress, parserGroupGress, deparserGroupGress,
                                     parserExtractGroupSource, c.size() - used));
        }
    }
    return rst;
}

/** Create an allocation from a vector of container IDs.  Physical
 * containers that the Device pins to a particular gress are
 * initialized to that gress.
 */
PHV::ConcreteAllocation::ConcreteAllocation(const PhvInfo &phv, const PhvUse &uses,
                                            bitvec containers, bool isTrivial)
    : PHV::Allocation(phv, uses, isTrivial) {
    auto &phvSpec = Device::phvSpec();
    for (auto cid : containers) {
        PHV::Container c = phvSpec.idToContainer(cid);

        // Initialize container status with hard-wired gress info and
        // an empty alloc slice list.
        std::optional<gress_t> gress = std::nullopt;
        std::optional<gress_t> parserGroupGress = std::nullopt;
        std::optional<gress_t> deparserGroupGress = std::nullopt;
        if (phvSpec.ingressOnly()[phvSpec.containerToId(c)]) {
            gress = INGRESS;
            parserGroupGress = INGRESS;
            deparserGroupGress = INGRESS;
        } else if (phvSpec.egressOnly()[phvSpec.containerToId(c)]) {
            gress = EGRESS;
            parserGroupGress = EGRESS;
            deparserGroupGress = EGRESS;
        }
        container_status_i[c] = {gress,
                                 parserGroupGress,
                                 deparserGroupGress,
                                 {},
                                 PHV::Allocation::ContainerAllocStatus::EMPTY,
                                 PHV::Allocation::ExtractSource::NONE};
    }
    // set phv state
    for (const auto &f : phv) {
        for (const auto &alloc : f.get_alloc()) {
            allocate(alloc);
        }
    }
}

PHV::ConcreteAllocation::ConcreteAllocation(const PhvInfo &phv, const PhvUse &uses, bool isTrivial)
    : PHV::ConcreteAllocation::ConcreteAllocation(phv, uses, Device::phvSpec().physicalContainers(),
                                                  isTrivial) {}

/// @returns true if this allocation owns @p c.
bool PHV::ConcreteAllocation::contains(PHV::Container c) const {
    return container_status_i.find(c) != container_status_i.end();
}

std::optional<PHV::ActionSet> PHV::Allocation::getInitPoints(const PHV::AllocSlice &slice) const {
    if (!meta_init_points_i.count(slice)) return std::nullopt;
    return meta_init_points_i.at(slice);
}

std::optional<PHV::ActionSet> PHV::Transaction::getInitPoints(const PHV::AllocSlice &slice) const {
    if (meta_init_points_i.count(slice)) return meta_init_points_i.at(slice);
    return parent_i->getInitPoints(slice);
}

void PHV::Transaction::printMetaInitPoints() const {
    LOG5("\t\tTransaction: Getting init points for slices:");
    for (const auto &kv : meta_init_points_i)
        LOG5("\t\t  " << kv.first << " : " << kv.second.size());
    const Transaction *parentTransaction = dynamic_cast<const PHV::Transaction *>(parent_i);
    while (parentTransaction) {
        LOG5("\t\tAllocation: Getting init points for slices:");
        for (const auto &kv : parentTransaction->get_meta_init_points())
            LOG5("\t\t  " << kv.first << " : " << kv.second.size());
        parentTransaction = dynamic_cast<const PHV::Transaction *>(parentTransaction->parent_i);
    }
}

const PHV::Allocation::ContainerStatus *PHV::ConcreteAllocation::getStatus(
    const PHV::Container &c) const {
    auto it = container_status_i.find(c);
    if (it != container_status_i.end()) return &it->second;
    if (isTrivial) return &emptyContainerStatus;
    return nullptr;
}

PHV::Allocation::FieldStatus PHV::ConcreteAllocation::getStatus(const PHV::Field *f) const {
    if (field_status_i.find(f) != field_status_i.end()) return field_status_i.at(f);
    return {};
}

void PHV::ConcreteAllocation::foreach_slice(const PHV::Field *f,
                                            std::function<void(const AllocSlice &)> cb) const {
    if (auto it = field_status_i.find(f); it != field_status_i.end()) {
        for (const auto &slice : it->second) {
            cb(slice);
        }
    }
}

void PHV::ConcreteAllocation::deallocate(const ordered_set<PHV::AllocSlice> &slices) {
    ordered_set<PHV::Container> touched_conts;
    for (const auto &sl : slices) {
        auto c = sl.container();
        touched_conts.insert(c);
        BUG_CHECK(container_status_i.count(c) && container_status_i[c].slices.count(sl),
                  "slice does not seem to be allocated: %1%", sl);
        container_status_i[c].slices.erase(sl);
        field_status_i[sl.field()].erase(sl);
    }
    // TODO: This is still not 100% correct because if we just removed deparsed
    // slices of a container *c*, deparser group gress prop of all other containers in
    // the same deparser group might need to be cleared if c was the only one that has
    // deparsed slice.
    // update gress and status if container is empty after deallocation.
    const auto &phv_spec = Device::phvSpec();
    for (const auto &c : touched_conts) {
        if (!container_status_i[c].slices.empty()) continue;
        const unsigned cid = phv_spec.containerToId(c);
        container_status_i[c].alloc_status = ContainerAllocStatus::EMPTY;
        if (!phv_spec.ingressOnly()[cid] && !phv_spec.egressOnly()[cid]) {
            container_status_i[c].gress = std::nullopt;
            // reset parser group gress.
            container_status_i[c].parserGroupGress = std::nullopt;
            for (const unsigned other_cid : phv_spec.parserGroup(cid)) {
                if (other_cid == cid) continue;
                const auto other = phv_spec.idToContainer(other_cid);
                const auto other_parser_group_gress = this->parserGroupGress(other);
                container_status_i[c].parserGroupGress = other_parser_group_gress;
                break;
            }
            // reset deparser group gress.
            container_status_i[c].deparserGroupGress = std::nullopt;
            for (const unsigned other_cid : phv_spec.deparserGroup(cid)) {
                if (other_cid == cid) continue;
                const auto other = phv_spec.idToContainer(other_cid);
                const auto other_deparser_group_gress = this->deparserGroupGress(other);
                container_status_i[c].deparserGroupGress = other_deparser_group_gress;
                break;
            }
        }
    }
}

/// @returns the container status of @c and fails if @c is not present.
PHV::Allocation::GressAssignment PHV::Allocation::gress(const PHV::Container &c) const {
    const auto *status = this->getStatus(c);
    BUG_CHECK(status, "Trying to get gress for container %1% not in Allocation",
              cstring::to_cstring(c));
    return status->gress;
}

PHV::Allocation::GressAssignment PHV::Allocation::parserGroupGress(PHV::Container c) const {
    const auto *status = this->getStatus(c);
    BUG_CHECK(status, "Trying to get parser group gress for container %1% not in Allocation",
              cstring::to_cstring(c));
    return status->parserGroupGress;
}

PHV::Allocation::GressAssignment PHV::Allocation::deparserGroupGress(PHV::Container c) const {
    const auto *status = this->getStatus(c);
    BUG_CHECK(status, "Trying to get deparser group gress for container %1% not in Allocation",
              cstring::to_cstring(c));
    return status->deparserGroupGress;
}

PHV::Allocation::ContainerAllocStatus PHV::Allocation::alloc_status(PHV::Container c) const {
    const auto *status = this->getStatus(c);
    BUG_CHECK(status, "Trying to get allocation status for container %1% not in Allocation",
              cstring::to_cstring(c));
    return status->alloc_status;
}

PHV::Allocation::ExtractSource PHV::Allocation::parserExtractGroupSource(PHV::Container c) const {
    const auto *status = this->getStatus(c);
    BUG_CHECK(status,
              "Trying to get parser extract group source for container %1% not in Allocation",
              cstring::to_cstring(c));
    return status->parserExtractGroupSource;
}

PHV::Allocation::const_iterator PHV::Transaction::begin() const {
    P4C_UNIMPLEMENTED("Transaction::begin()");
}

PHV::Allocation::const_iterator PHV::Transaction::end() const {
    P4C_UNIMPLEMENTED("Transaction::end()");
}

// Returns the contents of this transaction *and* its parent.
ordered_set<PHV::AllocSlice> PHV::Allocation::slices(PHV::Container c) const {
    return slices(c, StartLen(0, int(c.type().size())));
}

void PHV::Allocation::foreach_slice(PHV::Container c,
                                    std::function<void(const PHV::AllocSlice &)> cb) const {
    foreach_slice(c, StartLen(0, int(c.type().size())), cb);
}

ordered_set<PHV::AllocSlice> PHV::Allocation::slices(PHV::Container c, int stage,
                                                     PHV::FieldUse access) const {
    return slices(c, StartLen(0, int(c.type().size())), stage, access);
}

void PHV::Allocation::foreach_slice(PHV::Container c, int stage, PHV::FieldUse access,
                                    std::function<void(const PHV::AllocSlice &)> cb) const {
    foreach_slice(c, StartLen(0, int(c.type().size())), stage, access, cb);
}

// Returns the contents of this transaction *and* its parent.
ordered_set<PHV::AllocSlice> PHV::Allocation::slices(PHV::Container c, le_bitrange range) const {
    ordered_set<PHV::AllocSlice> rv;
    foreach_slice(c, range, [&](const PHV::AllocSlice &slice) { rv.insert(slice); });
    return rv;
}

void PHV::Allocation::foreach_slice(PHV::Container c, le_bitrange range,
                                    std::function<void(const PHV::AllocSlice &)> cb) const {
    if (const auto *status = this->getStatus(c))
        for (auto &slice : status->slices)
            if (slice.container_slice().overlaps(range)) {
                cb(slice);
            }
}

ordered_set<PHV::AllocSlice> PHV::Allocation::slices(PHV::Container c, le_bitrange range, int stage,
                                                     PHV::FieldUse access) const {
    ordered_set<PHV::AllocSlice> rv;
    foreach_slice(c, range, stage, access, [&](const PHV::AllocSlice &slice) { rv.insert(slice); });
    return rv;
}

void PHV::Allocation::foreach_slice(PHV::Container c, le_bitrange range, int stage,
                                    PHV::FieldUse access,
                                    std::function<void(const PHV::AllocSlice &)> cb) const {
    foreach_slice(c, range, [&](const PHV::AllocSlice &slice) {
        if (slice.isLiveAt(stage, access)) {
            cb(slice);
        }
    });
}

const PHV::Allocation::ContainerStatus *PHV::Transaction::getStatus(const PHV::Container &c) const {
    // If a status exists in the transaction, then it includes info from the
    // parent.
    auto it = container_status_i.find(c);
    if (it != container_status_i.end()) return &it->second;

    // Otherwise, retrieve and cache parent info.
    const auto *parentStatus = parent_i->getStatus(c);
    if (parentStatus) container_status_i[c] = *parentStatus;
    return parentStatus;
}

PHV::Allocation::FieldStatus PHV::Transaction::getStatus(const PHV::Field *f) const {
    // DO NOT cache field_status_i like container_status_i because
    // container_status_i are always modified-by-copy, while this
    // field_status_i are not. This leads to a bug that when field_status_i
    // is modified in a parent transaction, children transactions can
    // only see part of the actual alloc slices of @p field.
    // Also, the performance improvement of caching is open to doubt.
    PHV::Allocation::FieldStatus rst;
    this->foreach_slice(f, [&](const AllocSlice &slice) { rst.insert(slice); });
    return rst;
}

void PHV::Transaction::foreach_slice(const PHV::Field *f,
                                     std::function<void(const AllocSlice &)> cb) const {
    // DO NOT cache field_status_i like container_status_i because
    // container_status_i are always modified-by-copy, while this
    // field_status_i are not. This leads to a bug that when field_status_i
    // is modified in a parent transaction, children transactions can
    // only see part of the actual alloc slices of @p field.
    // Also, the performance improvement of caching is open to doubt.
    assoc::hash_set<le_bitrange> range_seen;
    if (field_status_i.count(f)) {
        for (const auto &slice : field_status_i.at(f)) {
            range_seen.insert(slice.field_slice());
            cb(slice);
        }
    }
    parent_i->foreach_slice(f, [&](const AllocSlice &s) {
        if (range_seen.count(s.field_slice())) return;
        cb(s);
    });
}

ordered_set<PHV::AllocSlice> PHV::Allocation::slices(const PHV::Field *f, le_bitrange range) const {
    ordered_set<PHV::AllocSlice> rv;

    // Get status, which includes parent and child info.
    this->foreach_slice(f, range, [&](const PHV::AllocSlice &slice) { rv.insert(slice); });

    return rv;
}

void PHV::Allocation::foreach_slice(const PHV::Field *f, le_bitrange range,
                                    std::function<void(const AllocSlice &)> cb) const {
    // Get status, which includes parent and child info.
    this->foreach_slice(f, [&](const AllocSlice &slice) {
        if (slice.field_slice().overlaps(range)) cb(slice);
    });
}

ordered_set<PHV::AllocSlice> PHV::Allocation::slices(const PHV::Field *f, le_bitrange range,
                                                     int stage, PHV::FieldUse access) const {
    ordered_set<PHV::AllocSlice> rv;
    foreach_slice(f, range, stage, access, [&](const AllocSlice &slice) { rv.insert(slice); });
    return rv;
}

void PHV::Allocation::foreach_slice(const PHV::Field *f, le_bitrange range, int stage,
                                    PHV::FieldUse access,
                                    std::function<void(const AllocSlice &)> cb) const {
    foreach_slice(f, range, [&](const AllocSlice &slice) {
        if (slice.isLiveAt(stage, access)) {
            cb(slice);
        }
    });
}

ordered_map<PHV::Container, PHV::Allocation::ContainerStatus> PHV::Transaction::get_actual_diff()
    const {
    ordered_map<PHV::Container, PHV::Allocation::ContainerStatus> rst;
    const auto *parent = getParent();
    for (const auto &container_status : getTransactionStatus()) {
        const auto &c = container_status.first;
        const auto parent_status = parent->getStatus(c);
        auto copy = container_status.second;
        for (const auto &a : container_status.second.slices) {
            // filter out allocated slices.
            if (parent_status && parent_status->slices.count(a)) {
                copy.slices.erase(a);
            }
        }
        if (!copy.slices.empty()) {
            rst[c] = copy;
        }
    }
    return rst;
}

cstring PHV::Transaction::getTransactionDiff() const {
    std::stringstream ss;

    auto parent_tr = dynamic_cast<const PHV::Transaction *>(parent_i);
    if (!parent_tr) {
        BUG("Expecting to compare with another Transaction");
        ss << "Expecting to compare with another Transaction";
        return ss.str();
    }

    for (const auto &kv : getTransactionStatus()) {
        PHV::Container c = kv.first;

        const auto *parent_status = parent_tr->getStatus(c);
        BUG_CHECK(parent_status,
                  "Trying to get allocation status for container %1% not in Allocation",
                  cstring::to_cstring(c));
        const PHV::Allocation::ContainerStatus &tr_status = kv.second;

        bool cnt_header = false;
        for (const AllocSlice &slice : tr_status.slices) {
            if (!parent_status->slices.count(slice)) {
                if (!cnt_header) {
                    ss << "    Container:" << c << std::endl;
                    cnt_header = true;
                }
                ss << "        ++ " << slice << std::endl;
            }
        }
        for (const AllocSlice &slice : parent_status->slices) {
            if (!tr_status.slices.count(slice)) {
                if (!cnt_header) {
                    ss << "    Container:" << c << std::endl;
                    cnt_header = true;
                }
                ss << "        -- " << slice << std::endl;
            }
        }
    }

    for (const auto &kv : getMetaInitPoints()) {
        ss << "    *** Initialization Points ***" << std::endl;
        ss << "    " << kv.first << std::endl;
        for (auto action : kv.second) ss << "            |-->" << *action << std::endl;
    }

    for (const auto &kv : getInitWrites()) {
        ss << "    *** Initialization Writes ***" << std::endl;
        ss << "    " << *kv.first << std::endl;
        for (auto field : kv.second) ss << "        |-->" << *field << std::endl;
    }

    return ss.str();
}

cstring PHV::Transaction::getTransactionSummary() const {
    ordered_map<std::optional<gress_t>,
                ordered_map<PHV::Type, ordered_map<ContainerAllocStatus, int>>>
        alloc_status;

    // Compute status.
    for (const auto &kv : getTransactionStatus()) {
        PHV::Container c = kv.first;
        ContainerAllocStatus status = ContainerAllocStatus::EMPTY;
        bitvec allocatedBits;
        for (const auto &slice : kv.second.slices)
            allocatedBits |= bitvec(slice.container_slice().lo, slice.container_slice().size());
        if (allocatedBits == bitvec(0, c.size()))
            status = ContainerAllocStatus::FULL;
        else if (!allocatedBits.empty())
            status = ContainerAllocStatus::PARTIAL;
        alloc_status[container_status_i.at(c).gress][c.type()][status]++;
    }

    // Print container status.
    std::stringstream ss;
    ss << "TRANSACTION STATUS (Only for this transaction):" << std::endl;
    ss << boost::format("%1% %|10t| %2% %|20t| %3% %|30t| %4%\n") % "GRESS" % "TYPE" % "STATUS" %
              "COUNT";

    bool first_by_gress = true;
    auto gresses = std::vector<std::optional<gress_t>>({INGRESS, EGRESS, std::nullopt});
    auto statuses = {ContainerAllocStatus::EMPTY, ContainerAllocStatus::PARTIAL,
                     ContainerAllocStatus::FULL};
    for (auto gress : gresses) {
        first_by_gress = true;
        for (auto status : statuses) {
            for (auto type : Device::phvSpec().containerTypes()) {
                if (alloc_status[gress][type][status] == 0) continue;
                std::stringstream ss_gress;
                std::string s_status;
                ss_gress << gress;
                switch (status) {
                    case ContainerAllocStatus::EMPTY:
                        s_status = "EMPTY";
                        break;
                    case ContainerAllocStatus::PARTIAL:
                        s_status = "PARTIAL";
                        break;
                    case ContainerAllocStatus::FULL:
                        s_status = "FULL";
                        break;
                }
                ss << boost::format("%1% %|10t| %3% %|20t| %2% %|30t| %4%\n") %
                          (first_by_gress ? ss_gress.str() : "") % type.toString() % s_status %
                          alloc_status[gress][type][status];
                first_by_gress = false;
            }
        }
    }

    return ss.str();
}

bool PHV::AlignedCluster::operator==(const PHV::AlignedCluster &other) const {
    return kind_i == other.kind_i && slices_i == other.slices_i;
}

void PHV::AlignedCluster::set_cluster_id() { id_i = cluster_id_g++; }

void PHV::AlignedCluster::initialize_constraints() {
    exact_containers_i = 0;
    max_width_i = 0;
    num_constraints_i = 0;
    aggregate_size_i = 0;
    alignment_i = std::nullopt;
    hasDeparsedFields_i = false;

    for (auto &slice : slices_i) {
        // TODO: These constraints will be subsumed by deparser schema.
        exact_containers_i += slice.field()->exact_containers() ? 1 : 0;
        max_width_i = std::max(max_width_i, slice.size());
        aggregate_size_i += slice.size();
        gress_i = slice.gress();
        if (slice.field()->deparsed() || slice.field()->deparsed_to_tm())
            hasDeparsedFields_i = true;

        auto s_alignment = slice.alignment();
        if (alignment_i && s_alignment && *alignment_i != *s_alignment) {
            std::stringstream msg;
            msg << "Fields involved in the same MAU operations have conflicting PARDE alignment "
                << "requirements: " << *alignment_i << " and " << *s_alignment << std::endl;
            msg << "Fields in cluster:" << std::endl;
            for (auto &slice : slices_i) msg << "    " << slice << std::endl;
            msg << "Consider using @in_hash { ... } annotation around statements containing "
                   "the fields mentioned above."
                << std::endl;
            error("%1%", msg.str());
        } else if (!alignment_i && s_alignment) {
            alignment_i = s_alignment;
        }

        // TODO: This should probably live in the field object.
        if (slice.field()->deparsed()) num_constraints_i++;
        if (slice.field()->is_solitary()) num_constraints_i++;
        if (slice.field()->deparsed_bottom_bits()) num_constraints_i++;
        if (slice.field()->deparsed_top_bits()) num_constraints_i++;
        if (slice.field()->exact_containers()) num_constraints_i++;
    }
}

std::optional<le_bitrange> PHV::AlignedCluster::validContainerStartRange(
    PHV::Size container_size) const {
    le_bitrange container_slice = StartLen(0, int(container_size));
    LOG5("Computing valid container start range for cluster "
         << " for placement in " << container_slice << " slices of " << container_size
         << " containers.");

    // Compute the range of valid alignment of the first bit (low, little
    // Endian) of all cluster fields, which is the intersection of the valid
    // starting bit positions of each field in the cluster.
    le_bitinterval valid_start_interval = ZeroToMax();
    for (auto &slice : slices_i) {
        auto this_valid_start_range = validContainerStartRange(slice, container_size);
        if (!this_valid_start_range) return std::nullopt;

        LOG5("\tField slice " << slice << " has valid start interval of "
                              << *this_valid_start_range);

        valid_start_interval =
            valid_start_interval.intersectWith(toHalfOpenRange(*this_valid_start_range));
    }

    return toClosedRange(valid_start_interval);
}

/* static */
std::optional<le_bitrange> PHV::AlignedCluster::validContainerStartRange(
    const PHV::FieldSlice &slice, PHV::Size container_size) {
    le_bitrange container_slice = StartLen(0, int(container_size));
    LOG5("Computing valid container start range for cluster "
         << " for placement in " << container_slice << " slices of " << container_size
         << " containers.");

    // Compute the range of valid alignment of the first bit (low, little
    // Endian) of all cluster fields, which is the intersection of the valid
    // starting bit positions of each field in the cluster.
    bool has_deparsed_bottom_bits = false;
    bool has_deparsed_top_bits = false;
    le_bitinterval valid_start_interval = ZeroToMax();

    // If the field has deparsed bottom bits, and it includes the LSB for
    // the field, then all fields in the cluster will need to be aligned at
    // zero.
    if (slice.field()->deparsed_bottom_bits() && slice.range().lo == 0) {
        has_deparsed_bottom_bits = true;
        LOG5("\tSlice " << slice << " has deparsed bottom bits");
    }

    // If the field has deparsed top bits then it needs to be placed in the top 16b.
    if (slice.field()->deparsed_top_bits() && container_size == PHV::Size::b32) {
        has_deparsed_top_bits = true;
        LOG5("\tSlice " << slice << " has deparsed top bits");
    }

    BUG_CHECK(slice.size() <= int(container_size), "Slice size greater than container size");
    LOG5("\tField slice " << slice << " to be placed in container size " << container_size
                          << " slices has "
                          << (slice.validContainerRange() == ZeroToMax()
                                  ? "no"
                                  : cstring::to_cstring(slice.validContainerRange()))
                          << " alignment requirement.");

    // Intersect the valid range of this field with the container size.
    nw_bitrange container_range = StartLen(0, int(container_size));
    nw_bitinterval valid_interval = container_range.intersectWith(slice.validContainerRange());
    BUG_CHECK(!valid_interval.empty(),
              "Bad absolute container range; "
              "field slice %1% has valid container range %2%, which has no "
              "overlap with container range %3%",
              slice.field()->name, slice.validContainerRange(), container_range);

    // Mask to top bits
    if (has_deparsed_top_bits) {
        // network order, so top half is 0 .. container_size / 2
        valid_interval = valid_interval.intersectWith(0, int(container_size) / 2);
        nw_bitrange top_range = StartLen(0, int(container_size) / 2);
        BUG_CHECK(!valid_interval.empty(),
                  "Bad absolute container range; "
                  "field slice %1% has valid container range %2%, which has no "
                  "overlap with top range %3%",
                  slice.field()->name, slice.validContainerRange(), top_range);
    }

    // ...and converted to little Endian with respect to the coordinate
    // space formed by the container.
    le_bitrange valid_range =
        (*toClosedRange(valid_interval)).toOrder<Endian::Little>(int(container_size));

    // Convert from a range denoting a valid placement of the whole slice
    // to a range denoting the valid placement of the first (lo, little
    // Endian) bit of the field in the first slice.
    //
    // (We add 1 to reflect that the valid starting range for a field
    // includes its first bit.)
    valid_start_interval = valid_range.resizedToBits(valid_range.size() - slice.range().size() + 1)
                               .intersectWith(container_slice);

    LOG5("\tField slice " << slice << " has valid start interval of " << valid_start_interval);

    // If any field has this requirement, then all fields must be aligned
    // at (little Endian) 0 in each container.
    if (has_deparsed_bottom_bits && valid_start_interval.contains(0))
        return le_bitrange(StartLen(0, 1));
    else if (has_deparsed_bottom_bits)
        return std::nullopt;
    else
        return toClosedRange(valid_start_interval);
}

bool PHV::AlignedCluster::okIn(PHV::Kind kind) const { return kind_i <= kind; }

bitvec PHV::AlignedCluster::validContainerStart(PHV::Size container_size) const {
    std::optional<le_bitrange> opt_valid_start_range = validContainerStartRange(container_size);
    if (!opt_valid_start_range) return bitvec();
    auto valid_start_range = *opt_valid_start_range;
    bitvec rv = bitvec(valid_start_range.lo, valid_start_range.size());

    for (const auto &slice : *this) rv &= slice.getStartBits(container_size);

    // Account for relative alignment.
    if (this->alignment()) {
        bitvec bitInByteStarts;
        int idx(*this->alignment());
        while (idx < int(container_size)) {
            bitInByteStarts.setbit(idx);
            idx += 8;
        }
        rv &= bitInByteStarts;
    }

    return rv;
}

/* static */
bitvec PHV::AlignedCluster::validContainerStart(PHV::FieldSlice slice, PHV::Size container_size) {
    std::optional<le_bitrange> opt_valid_start_range =
        validContainerStartRange(slice, container_size);
    if (!opt_valid_start_range) return bitvec();
    auto valid_start_range = *opt_valid_start_range;

    if (!slice.alignment()) return bitvec(valid_start_range.lo, valid_start_range.size());

    // account for relative alignment
    int align_start = valid_start_range.lo;
    if (align_start % 8 != int(slice.alignment()->align)) {
        bool next_byte = align_start % 8 > int(slice.alignment()->align);
        align_start += slice.alignment()->align + (next_byte ? 8 : 0) - align_start % 8;
    }

    bitvec rv;
    while (valid_start_range.contains(align_start)) {
        rv.setbit(align_start);
        align_start += 8;
    }

    return rv;
}

bool PHV::AlignedCluster::contains(const PHV::Field *f) const {
    for (auto &s : slices_i)
        if (s.field() == f) return true;
    return false;
}

bool PHV::AlignedCluster::contains(const PHV::FieldSlice &s1) const {
    for (auto &s2 : slices_i)
        if (s1.field() == s2.field()) return true;
    return false;
}

std::optional<PHV::SliceResult<PHV::AlignedCluster>> PHV::AlignedCluster::slice(int pos) const {
    BUG_CHECK(pos >= 0, "Trying to slice cluster at negative position");
    PHV::SliceResult<PHV::AlignedCluster> rv;
    std::vector<PHV::FieldSlice> lo_slices;
    std::vector<PHV::FieldSlice> hi_slices;
    for (auto &slice : slices_i) {
        // Put slice in lo if pos is larger than the slice.
        if (slice.range().size() <= pos) {
            lo_slices.push_back(slice);
            rv.slice_map.emplace(PHV::FieldSlice(slice), std::make_pair(slice, std::nullopt));
            continue;
        }
        // Check whether the field in `slice` can be sliced.
        if (slice.field()->no_split_at(pos + slice.range().lo)) {
            return std::nullopt;
        }
        // Create new slices.
        le_bitrange lo_range = StartLen(slice.range().lo, pos);
        le_bitrange hi_range =
            StartLen(slice.range().lo + pos, slice.range().size() - lo_range.size());
        auto lo_slice = PHV::FieldSlice(slice, lo_range);
        auto hi_slice = PHV::FieldSlice(slice, hi_range);
        lo_slices.push_back(lo_slice);
        hi_slices.push_back(hi_slice);
        // Update the slice map.
        rv.slice_map.emplace(PHV::FieldSlice(slice), std::make_pair(lo_slice, hi_slice));
    }
    rv.lo = new PHV::AlignedCluster(kind_i, lo_slices);
    rv.hi = new PHV::AlignedCluster(kind_i, hi_slices);
    return rv;
}

PHV::RotationalCluster::RotationalCluster(ordered_set<PHV::AlignedCluster *> clusters)
    : clusters_i(clusters) {
    // Populate the field-->cluster map (slices_to_clusters_i).
    for (auto *cluster : clusters_i) {
        for (auto &slice : *cluster) {
            slices_to_clusters_i[slice] = cluster;
            slices_i.insert(slice);
        }
    }

    // Tally stats.
    kind_i = PHV::Kind::tagalong;
    for (auto *cluster : clusters_i) {
        // TODO: We'll need to update this for JBay.
        if (!cluster->okIn(kind_i)) kind_i = PHV::Kind::normal;
        if (cluster->exact_containers()) exact_containers_i++;
        gress_i = cluster->gress();
        max_width_i = std::max(max_width_i, cluster->max_width());
        num_constraints_i += cluster->num_constraints();
        aggregate_size_i += cluster->aggregate_size();
        hasDeparsedFields_i |= cluster->deparsed();
    }
}

bool PHV::RotationalCluster::operator==(const PHV::RotationalCluster &other) const {
    if (clusters_i.size() != other.clusters_i.size()) return false;

    for (auto *cluster : clusters_i) {
        bool found = false;
        for (auto *cluster2 : other.clusters_i) {
            if (*cluster == *cluster2) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}

bool PHV::RotationalCluster::okIn(PHV::Kind kind) const { return kind_i <= kind; }

bool PHV::RotationalCluster::contains(const PHV::Field *f) const {
    for (auto &kv : slices_to_clusters_i)
        if (f == kv.first.field()) return true;
    return false;
}

bool PHV::RotationalCluster::contains(const PHV::FieldSlice &slice) const {
    return slices_to_clusters_i.find(slice) != slices_to_clusters_i.end();
}

std::optional<PHV::SliceResult<PHV::RotationalCluster>> PHV::RotationalCluster::slice(
    int pos) const {
    BUG_CHECK(pos >= 0, "Trying to slice cluster at negative position");
    PHV::SliceResult<PHV::RotationalCluster> rv;
    ordered_set<PHV::AlignedCluster *> lo_clusters;
    ordered_set<PHV::AlignedCluster *> hi_clusters;
    for (auto *aligned_cluster : clusters_i) {
        auto new_clusters = aligned_cluster->slice(pos);
        if (!new_clusters) return std::nullopt;
        // Filter empty clusters, as some clusters may only have fields smaller
        // than pos.
        if (new_clusters->lo->slices().size()) lo_clusters.insert(new_clusters->lo);
        if (new_clusters->hi->slices().size()) hi_clusters.insert(new_clusters->hi);
        for (auto &kv : new_clusters->slice_map) rv.slice_map.emplace(kv.first, kv.second);
    }

    rv.lo = new PHV::RotationalCluster(lo_clusters);
    rv.hi = new PHV::RotationalCluster(hi_clusters);
    return rv;
}

PHV::SuperCluster::SuperCluster(ordered_set<const PHV::RotationalCluster *> clusters,
                                ordered_set<SliceList *> slice_lists)
    : clusters_i(std::move(clusters)), slice_lists_i(std::move(slice_lists)) {
    // Populate the field slice-->cluster map (slices_to_clusters_i)
    for (auto *rotational_cluster : clusters_i)
        for (auto *aligned_cluster : rotational_cluster->clusters())
            for (auto &slice : *aligned_cluster) slices_to_clusters_i[slice] = rotational_cluster;

    // Check that every field is present in some cluster.
    for (auto *slice_list : slice_lists_i) {
        for (auto &slice : *slice_list) {
            BUG_CHECK(slices_to_clusters_i.find(slice) != slices_to_clusters_i.end(),
                      "Trying to form cluster group with a slice list containing %1%, "
                      "which is not present in any cluster",
                      cstring::to_cstring(slice));
            slices_to_slice_lists_i[slice].insert(slice_list);
        }
    }

    // Tally stats.
    kind_i = PHV::Kind::tagalong;
    for (auto *cluster : clusters_i) {
        // TODO: We'll need to update this for JBay.
        if (!cluster->okIn(kind_i)) kind_i = PHV::Kind::normal;
        if (cluster->exact_containers()) exact_containers_i++;
        gress_i = cluster->gress();
        max_width_i = std::max(max_width_i, cluster->max_width());
        num_constraints_i += cluster->num_constraints();
        aggregate_size_i += cluster->aggregate_size();
        hasDeparsedFields_i |= cluster->deparsed();
    }
}

void PHV::SuperCluster::calc_pack_conflicts() {
    // calculate number of no pack conditions for each super cluster
    forall_fieldslices([&](const PHV::FieldSlice &fs) {
        num_pack_conflicts_i += fs.field()->num_pack_conflicts();
    });
}

bool PHV::SuperCluster::operator==(const PHV::SuperCluster &other) const {
    if (clusters_i.size() != other.clusters_i.size() ||
        slice_lists_i.size() != other.slice_lists_i.size()) {
        return false;
    }

    for (auto *cluster : clusters_i) {
        bool found = false;
        for (auto *cluster2 : other.clusters_i) {
            if (*cluster == *cluster2) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    for (auto *list : slice_lists_i) {
        bool found = false;
        for (auto *list2 : other.slice_lists_i) {
            if (*list == *list2) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}

const ordered_set<const PHV::SuperCluster::SliceList *> &PHV::SuperCluster::slice_list(
    const PHV::FieldSlice &slice) const {
    static const ordered_set<const SliceList *> empty;
    if (slices_to_slice_lists_i.find(slice) == slices_to_slice_lists_i.end()) return empty;
    return slices_to_slice_lists_i.at(slice);
}

bool PHV::SuperCluster::okIn(PHV::Kind kind) const { return kind_i <= kind; }

PHV::SuperCluster *PHV::SuperCluster::merge(const SuperCluster *other) const {
    ordered_set<const RotationalCluster *> new_clusters;
    ordered_set<SliceList *> new_slice_lists;
    for (auto *sc : {this, other}) {
        for (auto *sl : sc->slice_lists()) {
            new_slice_lists.insert(sl);
        }
        for (auto *rot : sc->clusters()) {
            new_clusters.insert(rot);
        }
    }
    return new SuperCluster(new_clusters, new_slice_lists);
}

PHV::SuperCluster *PHV::SuperCluster::mergeAndSortBasedOnWideArith(
    const PHV::SuperCluster *sc) const {
    std::list<SliceList *> non_wide_slice_lists;
    std::list<SliceList *> wide_lo_slice_lists;
    std::list<SliceList *> wide_hi_slice_lists;

    auto processSliceList = [&](const ordered_set<SliceList *> *sl) {
        for (auto *a_list : *sl) {
            bool w = false;
            bool wLo = false;
            for (auto &fs : *a_list) {
                if (fs.field()->bit_used_in_wide_arith(fs.range().lo)) {
                    w = true;
                    wLo = fs.field()->bit_is_wide_arith_lo(fs.range().lo);
                    if (wLo)
                        wide_lo_slice_lists.push_back(a_list);
                    else
                        wide_hi_slice_lists.push_back(a_list);
                    break;
                }
            }
            if (!w) {
                non_wide_slice_lists.push_back(a_list);
            }
        }
    };
    processSliceList(&slice_lists_i);
    processSliceList(&sc->slice_lists_i);

    // Sort all the slice lists with wide arithmetic requirements
    // such that least significant bits of a field slice appear
    // earlier in the list, and the paired field slices are
    // adjacent in the list.
    // When we assign slices to containers, we need to consider
    // them in order so some other slice doesn't swoop in and take
    // its required location.
    std::list<SliceList *> wide_slice_lists;
    for (auto *lo_slice : wide_lo_slice_lists) {
        wide_slice_lists.push_back(lo_slice);
        // find its linked hi slice list
        auto *hi_slice = sc->findLinkedWideArithSliceList(lo_slice);
        // Possible that the hi_slice may be a part of the current supercluster, and not the passed
        // parameter.
        if (!hi_slice) hi_slice = findLinkedWideArithSliceList(lo_slice);
        BUG_CHECK(hi_slice, "Unable to find linked hi slice for %1%",
                  cstring::to_cstring(lo_slice));
        wide_slice_lists.push_back(hi_slice);
    }
    ordered_set<const RotationalCluster *> new_clusters_i;
    ordered_set<SliceList *> new_slice_lists;
    for (auto c : clusters_i) {
        new_clusters_i.insert(c);
    }
    for (auto *c2 : sc->clusters_i) {
        new_clusters_i.insert(c2);
    }
    for (auto sl : wide_slice_lists) {
        new_slice_lists.insert(sl);
    }
    for (auto sl : non_wide_slice_lists) {
        new_slice_lists.insert(sl);
    }
    return new SuperCluster(new_clusters_i, new_slice_lists);
}

bool PHV::SuperCluster::needToMergeForWideArith(const SuperCluster *sc) const {
    for (auto *list : slice_lists()) {
        for (auto &fs : *list) {
            int lo1 = fs.range().lo;
            if (fs.field()->bit_used_in_wide_arith(lo1)) {
                for (auto *list2 : sc->slice_lists()) {
                    for (auto &fs2 : *list2) {
                        int lo2 = fs2.range().lo;
                        if (lo1 != lo2 && fs.field() == fs2.field()) {
                            if (fs2.field()->bit_used_in_wide_arith(lo2)) {
                                if (((lo1 + 32) == lo2) || (lo2 + 32) == lo1) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

PHV::SuperCluster::SliceList *PHV::SuperCluster::findLinkedWideArithSliceList(
    const PHV::SuperCluster::SliceList *sl) const {
    // if a particular field slice participates in wide arithmetic,
    // search for its linked SliceList (either hi or lo)
    for (auto &fs : *sl) {
        int slice_lsb = fs.range().lo;
        if (fs.field()->bit_used_in_wide_arith(slice_lsb)) {
            bool lo_slice = false;
            if (fs.field()->bit_is_wide_arith_lo(fs.range().lo)) lo_slice = true;
            LOG7("Looking for slice list " << sl);
            LOG7("   lo_slice?  " << lo_slice);
            LOG7("   slice_lsb = " << slice_lsb);
            for (auto *list : slice_lists()) {
                for (auto &fs2 : *list) {
                    int slice_lsb2 = fs2.range().lo;
                    LOG7("   Looking for slice list " << list);
                    LOG7("      slice_lsb2 = " << slice_lsb2);
                    if (fs2.field()->bit_used_in_wide_arith(slice_lsb2)) {
                        if (0 == std::strcmp(fs.field()->name, fs2.field()->name)) {
                            if (lo_slice && (slice_lsb + 32) == slice_lsb2)
                                return list;
                            else if (!lo_slice && (slice_lsb - 32) == slice_lsb2)
                                return list;
                        }
                    }
                }
            }
        }
    }
    return nullptr;  // not linked (or not wide arith slice)
}

bool PHV::SuperCluster::contains(const PHV::Field *f) const {
    for (auto *cluster : clusters_i)
        if (cluster->contains(f)) return true;
    return false;
}

bool PHV::SuperCluster::contains(const PHV::FieldSlice &slice) const {
    for (auto *cluster : clusters_i)
        if (cluster->contains(slice)) return true;
    return false;
}

ordered_set<PHV::FieldSlice> PHV::SuperCluster::slices() const {
    ordered_set<PHV::FieldSlice> rst;
    for (auto *cluster : clusters_i) {
        for (auto sl : cluster->slices()) rst.insert(sl);
    }
    return rst;
}

bool PHV::SuperCluster::is_deparser_zero_candidate() const {
    return all_of_fieldslices(
        [](const PHV::FieldSlice &fs) { return fs.field()->is_deparser_zero_candidate(); });
}

/// @returns true if all slices lists and slices are smaller than 32b and no
/// slice list contains more than one slice per aligned cluster.
/// TODO: Also check that slice lists with exact_container requirements
/// are all the same size.  We should check this ahead of time, though.
/// TODO: Also check that deparsed bottom bits fields are at the front of
/// their slice lists.
bool PHV::SuperCluster::is_well_formed(const SuperCluster *sc, PHV::Error *err) {
    err->set(PHV::ErrorCode::unknown);
    std::map<int, const PHV::SuperCluster::SliceList *> exact_list_sizes;
    int widest = 0;
    for (auto *list : sc->slice_lists()) {
        // // TODO: marking solitary and isClearOnWrite for multiple-write field
        // // is the hack we use today. Seeing different fields with solitary constraints,
        // // in one slice list is pretty weird. Here we hack it to allow this case.
        // // check solitary constraints for all slice list.
        // auto solitary_field = std::make_optional<const PHV::Field*>(false, nullptr);
        // bool all_clear_on_write_solitary = true;
        // for (const auto& sl : *list) {
        //     if (sl.field()->is_solitary()) {
        //         solitary_field = sl.field();
        //         all_clear_on_write_solitary &=
        //             sl.field()->getSolitaryConstraint().isClearOnWrite();
        //     }
        // }
        // if (solitary_field && !all_clear_on_write_solitary) {
        //     // except for one solitary field, others need all to be padding fields.
        //     for (const auto& sl : *list) {
        //         if (!sl.field()->padding && sl.field() != *solitary_field) {
        //             *err << "multiple different solitary fields in one slice list: " << list;
        //             return false;
        //         }
        //     }
        // }
        ordered_set<const PHV::AlignedCluster *> seen;
        int size = 0;
        bool has_exact_containers_or_deparsed = false;
        for (auto &slice : *list) {
            if (slice.field()->deparsed_bottom_bits() && slice.range().lo == 0 && size != 0) {
                *err << "slice at offset " << size << " has deparsed_bottom_bits: " << slice;
                return false;
            }
            has_exact_containers_or_deparsed |=
                slice.field()->exact_containers() || slice.field()->deparsed();
            size += slice.size();
            // Check that slice lists do not contain slices from the same
            // AlignedCluster.
            auto *cluster = &sc->aligned_cluster(slice);
            if (seen.find(cluster) != seen.end()) {
                *err << "but slice list has two slices from the same aligned cluster: \n";
                *err << list;
                return false;
            }
            seen.insert(cluster);
        }
        if (has_exact_containers_or_deparsed) {
            const auto is_deparsed_or_digested = [](const PHV::FieldSlice &fs) {
                const auto *f = fs.field();
                return (f->deparsed() || f->exact_containers() || f->is_digest());
            };
            if (!std::all_of(list->begin(), list->end(), is_deparsed_or_digested)) {
                *err << "Mix of deparsed/not deparsed fields cannot be placed together: " << list;
                return false;
            }
        }
        // TODO: a slice list like below: actually requires > 8 bit container because
        // 6(the alignment constraint) + 6 (the size of the field) = 12 > 8.
        // [ ingress::Cassa.Dairyland.Osterdock<6> ^6 ^bit[0..9] meta [0:5] ]
        if (auto alignment = list->front().alignment()) {
            widest = std::max(widest, int((*alignment).align) + size);
        } else {
            widest = std::max(widest, size);
        }

        if (has_exact_containers_or_deparsed) exact_list_sizes[size] = list;
        if (size > int(PHV::Size::b32)) {
            *err << "    ...but 32 < " << list << "\n";
            return false;
        }
    }

    // Check the widths of slices in RotationalClusters, which could be wider
    // than fields in slice lists in the case of non-uniform operand widths.
    for (auto *rotational : sc->clusters())
        for (auto *aligned : rotational->clusters())
            for (auto &slice : *aligned) widest = std::max(widest, slice.size());

    // Check that all slice lists with exact container requirements are the
    // same size.
    if (exact_list_sizes.size() > 1) {
        err->set(PHV::ErrorCode::slicelist_sz_mismatch);
        *err << "    ...but slice lists with 'exact container' constraints differ in size\n";
        for (const auto &kv : exact_list_sizes) {
            *err << "total: " << kv.first << ", : " << *kv.second << "\n";
        }
        return false;
    }

    // Check that nothing is wider than the widest slice list with exact
    // container requirements.
    if (exact_list_sizes.size() > 0 && widest > (*exact_list_sizes.begin()).first) {
        err->set(PHV::ErrorCode::slicelist_sz_mismatch);
        *err << "    ...but supercluster contains a slice/slice list wider "
                "than a slice list with the 'exact container' constraint\n";
        for (const auto &kv : exact_list_sizes) {
            *err << "total: " << kv.first << ", : " << *kv.second << "\n";
        }
        return false;
    }

    for (auto *rotational : sc->clusters())
        for (auto *aligned : rotational->clusters())
            for (auto &slice : *aligned)
                if (slice.size() > int(PHV::Size::b32)) {
                    *err << "    ...but 32 < " << slice;
                    return false;
                }

    err->set(PHV::ErrorCode::ok);
    return true;
}

int PHV::SuperCluster::slice_list_total_bits(const SliceList &list) {
    auto alloc_function = [](int a, const PHV::FieldSlice &b) { return a + b.size(); };
    // Accumuate the bit size of the slice list
    int slice_bit_width = std::accumulate(list.begin(), list.end(), 0, alloc_function);
    return slice_bit_width;
}

std::vector<le_bitrange> PHV::SuperCluster::slice_list_exact_containers(const SliceList &list) {
    auto ret_vector = std::vector<le_bitrange>();
    for (const auto &field_slice : list) {
        if (!field_slice.field()->exact_containers()) continue;
        ret_vector.push_back(field_slice.range());
    }
    return ret_vector;
}

bool PHV::SuperCluster::slice_list_has_exact_containers(const SliceList &list) {
    return slice_list_exact_containers(list).size() > 0;
}

std::vector<PHV::SuperCluster::SliceList *> PHV::SuperCluster::slice_list_split_by_byte(
    const SuperCluster::SliceList &sl) {
    if (sl.empty()) return {};
    const auto &head = sl.front();
    int offset = head.alignment() ? head.alignment()->align : 0;
    auto curr = new SuperCluster::SliceList();

    std::vector<SuperCluster::SliceList *> rst;
    std::deque<PHV::FieldSlice> slices(sl.begin(), sl.end());
    while (!slices.empty()) {
        FieldSlice fs(slices.front().field(), slices.front().range());
        slices.pop_front();
        if (offset + fs.size() >= 8) {
            const int head_length = 8 - offset;
            auto head = FieldSlice(fs, StartLen(fs.range().lo, head_length));
            curr->push_back(head);
            rst.push_back(curr);
            offset = 0;
            curr = new SuperCluster::SliceList();
            if (fs.size() - head_length > 0) {
                auto tail =
                    FieldSlice(fs, StartLen(fs.range().lo + head_length, fs.size() - head_length));
                slices.push_front(tail);
            }
        } else {
            offset += fs.size();
            curr->push_back(fs);
        }
    }
    if (curr->size() > 0) {
        rst.push_back(curr);
    }
    return rst;
}

namespace PHV {

std::ostream &operator<<(std::ostream &out, const PHV::Allocation &alloc) {
    if (dynamic_cast<const PHV::Transaction *>(&alloc)) {
        P4C_UNIMPLEMENTED("<<(PHV::Transaction)");
        return out;
    }

    AllocationReport report(alloc);
    out << report.printSummary() << std::endl;

    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::Allocation *alloc) {
    if (alloc)
        out << *alloc;
    else
        out << "-null-alloc-";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::Allocation::ExtractSource &source) {
    using ExtractSource = PHV::Allocation::ExtractSource;
    switch (source) {
        case ExtractSource::NONE:
            out << "none";
            break;
        case ExtractSource::PACKET:
            out << "packet";
            break;
        case ExtractSource::NON_PACKET:
            out << "non-packet";
            break;
        default:
            out << "unknown";
            break;
    };
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::ContainerGroup &g) {
    out << "(";
    auto it = g.begin();
    while (it != g.end()) {
        out << *it;
        ++it;
        if (it != g.end()) out << ", ";
    }
    out << ")";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::ContainerGroup *g) {
    if (g)
        out << *g;
    else
        out << "-null-container-group-";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::AlignedCluster &cl) {
    out << "[";
    unsigned count = 0;
    for (auto &slice : cl) {
        count++;
        out << slice;
        if (count < cl.slices().size()) out << ", ";
    }
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::AlignedCluster *cl) {
    if (cl)
        out << *cl;
    else
        out << "-null-aligned-cluster-";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::RotationalCluster &cl) {
    out << "[";
    unsigned count = 0;
    for (auto &cluster : cl.clusters()) {
        count++;
        out << cluster;
        if (count < cl.clusters().size()) out << ", ";
    }
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::RotationalCluster *cl) {
    if (cl)
        out << *cl;
    else
        out << "-null-rotational-cluster-";
    return out;
}

// TODO: This could really stand to be improved.
std::ostream &operator<<(std::ostream &out, const PHV::SuperCluster &g) {
    // Print the slice lists.
    out << "SUPERCLUSTER Uid: " << g.uid << std::endl;
    out << "    slice lists:\t";
    if (g.slice_lists().size() == 0) {
        out << "[ ]" << std::endl;
    } else {
        out << std::endl;
        for (auto *slice_list : g.slice_lists()) {
            out << boost::format("%|8t|%1%") % cstring::to_cstring(*slice_list);
            out << std::endl;
        }
    }

    // Print aligned clusters.
    out << "    rotational clusters:\t";
    if (g.clusters().size() == 0) {
        out << "{ }" << std::endl;
    } else {
        out << std::endl;
        for (auto *cluster : g.clusters()) {
            out << boost::format("%|8t|%1%") % cstring::to_cstring(cluster);
            out << std::endl;
        }
    }

    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::SuperCluster *g) {
    if (g)
        out << *g;
    else
        out << "-null-cluster-group-";
    return out;
}

std::ostream &operator<<(std::ostream &out, const SuperCluster::SliceList &list) {
    out << "[";
    for (auto &slice : list) {
        if (slice == list.front())
            out << " " << slice;
        else
            out << "          " << slice;
        if (slice != list.back()) out << std::endl;
    }
    out << " ]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const SuperCluster::SliceList *list) {
    if (list)
        out << *list;
    else
        out << "-null-slice-list-";
    return out;
}

/// Partial order for allocation status.
bool operator<(PHV::Allocation::ContainerAllocStatus left,
               PHV::Allocation::ContainerAllocStatus right) {
    if (left == right)
        return false;
    else if (right == PHV::Allocation::ContainerAllocStatus::EMPTY)
        return false;
    else if (right == PHV::Allocation::ContainerAllocStatus::PARTIAL &&
             left == PHV::Allocation::ContainerAllocStatus::FULL)
        return false;
    else
        return true;
}

bool operator<=(PHV::Allocation::ContainerAllocStatus left,
                PHV::Allocation::ContainerAllocStatus right) {
    return left < right || left == right;
}

bool operator>(PHV::Allocation::ContainerAllocStatus left,
               PHV::Allocation::ContainerAllocStatus right) {
    return !(left <= right);
}

bool operator>=(PHV::Allocation::ContainerAllocStatus left,
                PHV::Allocation::ContainerAllocStatus right) {
    return !(left < right);
}

}  // namespace PHV
