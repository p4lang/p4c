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

#include "bf-p4c/phv/utils/report.h"

#include <numeric>

#include "bf-p4c/common/table_printer.h"

using ContainerAllocStatus = PHV::Allocation::ContainerAllocStatus;

void PHV::AllocationReport::collectStatus() {
    // TODO: code duplication of available_spots()
    // Compute status.
    for (const auto &kv : alloc) {
        PHV::Container c = kv.first;
        ContainerAllocStatus status = ContainerAllocStatus::EMPTY;
        container_to_bits_used[c] = 0;
        container_to_bits_allocated[c] = 0;
        bitvec allocatedBits;
        auto &slices = kv.second.slices;

        auto gress = kv.second.gress;
        container_to_gress[c] = gress;

        for (auto slice : slices) {
            allocatedBits |= bitvec(slice.container_slice().lo, slice.container_slice().size());
            total_allocated_bits += slice.width();
            container_to_bits_allocated[c] += slice.width();
            alloc_slices.insert(slice);
        }

        if (allocatedBits == bitvec(0, c.size())) {
            status = ContainerAllocStatus::FULL;
            container_to_bits_used[c] = c.size();
        } else if (!allocatedBits.empty()) {
            int used = std::accumulate(allocatedBits.begin(), allocatedBits.end(), 0,
                                       [](int a, int) { return a + 1; });
            partial_containers_stat[c] = c.size() - used;
            container_to_bits_used[c] = used;
            total_unallocated_bits += partial_containers_stat[c];
            if (slices.size() > 1 ||
                (slices.size() == 1 && !slices.begin()->field()->is_solitary())) {
                if (gress && *gress == INGRESS) {
                    valid_ingress_unallocated_bits += partial_containers_stat[c];
                } else if (gress && *gress == EGRESS) {
                    valid_egress_unallocated_bits += partial_containers_stat[c];
                }
            }
            status = ContainerAllocStatus::PARTIAL;
        }

        alloc_status[(*alloc.getStatus(c)).gress][c.type()][status]++;
    }
}

static std::string format_bit_slice(unsigned lo, unsigned hi) {
    std::stringstream slice;
    slice << "[" << hi;
    if (lo != hi) slice << ":" << lo;
    slice << "]";
    return slice.str();
}

static std::pair<std::string, std::string> format_alloc_slice(const PHV::AllocSlice &slice) {
    std::stringstream container_slice;
    std::stringstream field_slice;
    if (slice.container_slice().size() != int(slice.container().size()))
        container_slice << format_bit_slice(slice.container_slice().lo, slice.container_slice().hi);
    if (slice.field_slice().size() != slice.field()->size)
        field_slice << format_bit_slice(slice.field_slice().lo, slice.field_slice().hi);
    if (LOGGING(4)) {
        field_slice << " {" << slice.getEarliestLiveness().first
                    << slice.getEarliestLiveness().second;
        field_slice << ", " << slice.getLatestLiveness().first << slice.getLatestLiveness().second
                    << "}";
    }

    return {container_slice.str(), field_slice.str()};
}

std::vector<std::string> getPrinterSliceRow(const PHV::AllocSlice &slice,
                                            const std::optional<gress_t> &gress, bool first,
                                            bool hardwired = false) {
    auto formatted = format_alloc_slice(slice);
    std::string is_ara("");
    if (slice.getInitPrimitive().isAlwaysRunActionPrim()) is_ara += " ARA";

    return {first ? std::string(slice.container().toString()) : "",
            first ? (std::string(toSymbol(*gress)) + (hardwired ? "-HW" : "")) : "",
            formatted.first, std::string(slice.field()->name + formatted.second + is_ara)};
}

cstring PHV::AllocationReport::printAllocation() const {
    std::stringstream out;

    auto &phvSpec = Device::phvSpec();

    // Use this vector to control the order in which containers are printed,
    // i.e. MAU groups (ordered by group) first.
    std::vector<bitvec> order;

    // Track container IDs already in order.
    bitvec seen;

    // Print containers by MAU group, then TPHV collection, then by ID for any
    // remaining.
    for (auto typeAndGroup : phvSpec.mauGroups()) {
        for (auto group : typeAndGroup.second) {
            seen |= group;
            order.push_back(group);
        }
    }
    for (auto tagalong : phvSpec.tagalongCollections()) {
        seen |= tagalong;
        order.push_back(tagalong);
    }
    order.push_back(phvSpec.physicalContainers() - seen);

    out << "PHV Allocation" << std::endl;
    TablePrinter tp(out, {"Container", "Gress", "Container Slice", "Field Slice"},
                    TablePrinter::Align::LEFT);

    std::map<gress_t, std::map<PHV::Container, std::vector<PHV::AllocSlice>>> pov_bits;

    bool firstEmpty = true;
    for (bitvec group : order) {
        for (auto cid : group) {
            auto container = phvSpec.idToContainer(cid);
            auto slices = alloc.slices(container);
            auto gress = alloc.gress(container);
            bool hardwired = phvSpec.ingressOnly()[cid] || phvSpec.egressOnly()[cid];

            if (slices.size() == 0) {
                if (firstEmpty) {
                    tp.addRow({"...", "", "", ""});
                    tp.addBlank();
                    firstEmpty = false;
                }
                continue;
            }
            firstEmpty = true;

            bool first = true;
            for (const auto &slice : slices) {
                tp.addRow(getPrinterSliceRow(slice, gress, first, hardwired));
                first = false;

                if (slice.field()->pov) {
                    pov_bits[*gress][container].push_back(slice);
                }
            }
            tp.addBlank();
        }
    }

    tp.print();
    out << std::endl;

    unsigned total_available_bits = Device::phvSpec().getNumPovBits();

    for (auto &gp : pov_bits) {
        out << std::endl << "POV Allocation (" << toString(gp.first) << "):" << std::endl;
        TablePrinter tp2(out, {"Container", "Container Slice", "Field Slice"},
                         TablePrinter::Align::LEFT);

        unsigned total_container_bits = 0, total_used_bits = 0;

        for (auto &cs : gp.second) {
            auto &container = cs.first;

            bool firstSlice = true;
            for (auto &slice : cs.second) {
                auto formatted = format_alloc_slice(slice);

                tp2.addRow({firstSlice ? std::string(container.toString()) : "", formatted.first,
                            std::string(slice.field()->name) + formatted.second});

                firstSlice = false;
            }

            total_container_bits += container.size();
            total_used_bits += cs.second.size();
            tp2.addSep();
        }

        tp2.addRow(
            {"", "Total Bits Used", formatUsage(total_used_bits, total_available_bits, true)});

        tp2.addRow({"", "Pack Density", formatUsage(total_used_bits, total_container_bits, true)});

        tp2.print();
    }

    return out.str();
}

cstring PHV::AllocationReport::printAllocSlices() const {
    std::stringstream out;

    TablePrinter tp(out,
                    {"Field Slice", "Live Range", "Container", "Container Type", "Container Slice"},
                    TablePrinter::Align::LEFT);

    for (auto slice = alloc_slices.sorted_begin(); slice != alloc_slices.sorted_end(); slice++) {
        auto formatted = format_alloc_slice(*slice);
        std::stringstream lr_str;
        lr_str << (slice->isPhysicalStageBased() ? "P" : "") << "[" << slice->getEarliestLiveness()
               << ", " << slice->getLatestLiveness() << "]";
        tp.addRow({
            std::string(slice->field()->name) + formatted.second,
            lr_str.str(),
            std::string(slice->container().toString()),
            std::string(slice->container().type().toString()),
            formatted.first,
        });
    }

    tp.print();
    out << std::endl;
    return out.str();
}

cstring PHV::AllocationReport::printOverlayStatus() const {
    std::stringstream ss;

    // Compute overlay status.
    ordered_map<PHV::Container, int> overlay_result;
    int overlay_statistics[2][2] = {{0}};
    for (auto kv : alloc) {
        PHV::Container c = kv.first;
        int n_overlay = -c.size();
        for (auto i = c.lsb(); i <= c.msb(); ++i) {
            const auto &slices = kv.second.slices;
            n_overlay += std::accumulate(slices.begin(), slices.end(), 0,
                                         [&](int l, const PHV::AllocSlice &r) {
                                             return l + r.container_slice().contains(i);
                                         });
        }
        if (n_overlay > 0) {
            overlay_result[c] = n_overlay;
            if (c.type().kind() == PHV::Kind::tagalong) {
                overlay_statistics[*kv.second.gress][0] += n_overlay;
            } else {
                overlay_statistics[*kv.second.gress][1] += n_overlay;
            }
        }
    }

    if (LOGGING(2)) {
        ss << "======== CONTAINER OVERLAY STAT ===========" << std::endl;
        ss << "TOTAL INGRESS T-PHV OVERLAY BITS: " << overlay_statistics[INGRESS][0] << std::endl;
        ss << "TOTAL INGRESS PHV OVERLAY BITS: " << overlay_statistics[INGRESS][1] << std::endl;
        ss << "TOTAL EGRESS T-PHV OVERLAY BITS: " << overlay_statistics[EGRESS][0] << std::endl;
        ss << "TOTAL EGRESS PHV OVERLAY BITS: " << overlay_statistics[EGRESS][1] << std::endl;

        if (LOGGING(3) && !overlay_result.empty()) {
            TablePrinter tp(ss, {"Container", "Gress", "Container Slice", "Field Slice"},
                            TablePrinter::Align::LEFT);

            for (auto &kv : overlay_result) {
                auto container = kv.first;
                auto gress = container_to_gress.at(container);

                bool first = true;
                for (const auto &slice : alloc.slices(kv.first)) {
                    tp.addRow(getPrinterSliceRow(slice, gress, first));
                    first = false;
                }

                tp.addRow({"", "", "", std::to_string(kv.second) + " bits overlaid"});
                tp.addBlank();
            }

            tp.print();
        }

        // Output Partial Containers
        ss << "======== PARTIAL CONTAINERS STAT ==========" << std::endl;
        ss << "TOTAL UNALLOCATED BITS: " << total_unallocated_bits << std::endl;
        ss << "VALID INGRESS UNALLOCATED BITS: " << valid_ingress_unallocated_bits << std::endl;
        ss << "VALID EGRESS UNALLOCATED BITS: " << valid_egress_unallocated_bits << std::endl;

        if (LOGGING(3)) {
            for (const auto &kv : partial_containers_stat) {
                ss << kv.first << " has unallocated bits: " << kv.second << std::endl;
                for (const auto &slice : alloc.slices(kv.first)) {
                    ss << slice << std::endl;
                }
            }
        }

        // compute tphv fields allocated on phv fields
        int total_tphv_on_phv = 0;
        ordered_map<PHV::Container, int> tphv_on_phv;
        for (auto kv : alloc) {
            PHV::Container c = kv.first;
            if (c.is(PHV::Kind::tagalong)) {
                continue;
            }
            int n_tphvs = 0;
            for (auto slice : kv.second.slices) {
                if (slice.field()->is_tphv_candidate(uses)) {
                    n_tphvs += slice.width();
                }
            }
            if (n_tphvs == 0) continue;
            tphv_on_phv[c] = n_tphvs;
            total_tphv_on_phv += n_tphvs;
        }

        ss << "======== TPHV ON PHV STAT ========" << std::endl;
        ss << "Total bits: " << total_tphv_on_phv << std::endl;

        if (LOGGING(3)) {
            for (auto kv : tphv_on_phv) {
                ss << kv.first << " has " << kv.second << " bits " << std::endl;
                for (const auto &slice : alloc.slices(kv.first)) {
                    if (slice.field()->is_tphv_candidate(uses)) {
                        ss << slice << std::endl;
                    }
                }
            }
        }
    }

    return ss.str();
}

cstring PHV::AllocationReport::printContainerStatus() {
    std::stringstream ss;

    if (LOGGING(3)) {
        // Print container status.
        ss << "CONTAINER STATUS (after allocation so far):" << std::endl;
        ss << boost::format("%1% %|10t| %2% %|20t| %3% %|30t| %4%\n") % "GRESS" % "TYPE" %
                  "STATUS" % "COUNT";

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
        ss << std::endl;
    }

    return ss.str();
}

cstring PHV::AllocationReport::printMauGroupsOccupancyMetrics() const {
    const auto &phvSpec = Device::phvSpec();

    ordered_map<bitvec, MauGroupInfo> mauGroupInfos;

    for (auto cid : phvSpec.physicalContainers()) {
        PHV::Container c = phvSpec.idToContainer(cid);
        if (std::optional<bitvec> mauGroup = phvSpec.mauGroup(cid)) {
            // groupID represents the unique string that identifies an MAU group
            int groupID = phvSpec.mauGroupId(c);
            bool containerUsed = (container_to_bits_used.at(c) != 0);
            auto bits_used = container_to_bits_used.at(c);
            auto bits_allocated = container_to_bits_allocated.at(c);
            auto gress = container_to_gress.at(c);

            if (mauGroupInfos.count(*mauGroup)) {
                mauGroupInfos[*mauGroup].update(c.type().kind(), containerUsed, bits_used,
                                                bits_allocated, gress);
            } else {
                mauGroupInfos[*mauGroup] =
                    MauGroupInfo(c.size(), groupID, c.type().kind(), containerUsed, bits_used,
                                 bits_allocated, gress);
            }
        }
    }

    std::stringstream ss;
    ss << std::endl << "MAU Groups:" << std::endl;

    // Figure out what kinds of PHV containers to report. (Don't report on T-PHVs.)
    std::set<PHV::Kind> containerKinds(phvSpec.containerKinds());
    containerKinds.erase(PHV::Kind::tagalong);

    TablePrinter tp(ss,
                    {containerKinds.size() == 1 ? "MAU Group" : "Container Set", "Containers Used",
                     "Bits Used", "Bits Used on Ingress", "Bits Used on Egress", "Bits Allocated",
                     "Bits Allocated on Ingress", "Bits Allocated on Egress", "Available Bits"},
                    TablePrinter::Align::CENTER);

    PhvOccupancyMetric overallStats;
    auto overallTotalBits = 0;
    auto overallTotalContainers = 0;

    std::map<PHV::Size, PhvOccupancyMetric> statsByContainerSize;
    std::map<PHV::Size, unsigned> totalContainersBySize;

    std::map<PHV::Kind, PhvOccupancyMetric> statsByContainerKind;
    std::map<PHV::Kind, unsigned> totalBitsByKind;
    std::map<PHV::Kind, unsigned> totalContainersByKind;

    int groupNum = 1;

    for (auto containerSize : phvSpec.containerSizes()) {
        auto &sizeStats = statsByContainerSize[containerSize];

        bool firstGroup = true;
        for (auto mauGroup : phvSpec.mauGroups(containerSize)) {
            auto &info = mauGroupInfos[mauGroup];

            sizeStats += info.totalStats;
            totalContainersBySize[containerSize] += mauGroup.popcount();

            if (!firstGroup && containerKinds.size() > 1) {
                // We have more than one container kind in each group. Separate each group with a
                // line.
                tp.addSep();
            }

            firstGroup = false;

            // Print a line for each container kind in this group.
            for (auto &kv : info.statsByContainerKind) {
                auto &kind = kv.first;
                auto &kindInfo = kv.second;

                auto subgroup = phvSpec.filterContainerSet(mauGroup, kind);
                auto curTotalBits = subgroup.popcount() * size_t(containerSize);
                totalContainersByKind[kind] += subgroup.popcount();

                statsByContainerKind[kind] += kindInfo;
                totalBitsByKind[kind] += curTotalBits;
                overallTotalBits += curTotalBits;

                std::stringstream containerSet;
                containerSet << phvSpec.containerSetToString(subgroup);

                tp.addRow({containerSet.str(),
                           formatUsage(kindInfo.total.containersUsed, subgroup.popcount()),
                           formatUsage(kindInfo.total.bitsUsed, curTotalBits),
                           formatUsage(kindInfo.gress[INGRESS].bitsUsed, curTotalBits),
                           formatUsage(kindInfo.gress[EGRESS].bitsUsed, curTotalBits),
                           formatUsage(kindInfo.total.bitsAllocated, curTotalBits),
                           formatUsage(kindInfo.gress[INGRESS].bitsAllocated, curTotalBits),
                           formatUsage(kindInfo.gress[EGRESS].bitsAllocated, curTotalBits),
                           std::to_string(curTotalBits)});
            }

            if (containerKinds.size() > 1) {
                // Output a summary for the group.
                tp.addBlank();
                std::stringstream group;
                group << "Usage for Group " << groupNum++;

                auto totalStats = info.totalStats;
                auto totalContainers = mauGroup.popcount();
                auto curTotalBits = totalContainers * size_t(containerSize);

                tp.addRow({group.str(),
                           formatUsage(totalStats.total.containersUsed, totalContainers),
                           formatUsage(totalStats.total.bitsUsed, curTotalBits),
                           formatUsage(totalStats.gress[INGRESS].bitsUsed, curTotalBits),
                           formatUsage(totalStats.gress[EGRESS].bitsUsed, curTotalBits),
                           formatUsage(totalStats.total.bitsAllocated, curTotalBits),
                           formatUsage(totalStats.gress[INGRESS].bitsAllocated, curTotalBits),
                           formatUsage(totalStats.gress[EGRESS].bitsAllocated, curTotalBits),
                           std::to_string(curTotalBits)});
            }
        }

        if (phvSpec.mauGroups(containerSize).size() != 0) {
            tp.addSep();
        }

        overallStats += sizeStats;
        overallTotalContainers += totalContainersBySize[containerSize];
    }

    // Print stats for usage by container size.
    for (auto &kv : statsByContainerSize) {
        auto &size = kv.first;
        auto &stats = kv.second;

        auto totalContainers = totalContainersBySize.at(size);
        auto curTotalBits = totalContainers * size_t(size);

        std::stringstream ss;
        ss << "Usage for " << size_t(size) << "b";

        tp.addRow({ss.str(), formatUsage(stats.total.containersUsed, totalContainers),
                   formatUsage(stats.total.bitsUsed, curTotalBits),
                   formatUsage(stats.gress[INGRESS].bitsUsed, curTotalBits),
                   formatUsage(stats.gress[EGRESS].bitsUsed, curTotalBits),
                   formatUsage(stats.total.bitsAllocated, curTotalBits),
                   formatUsage(stats.gress[INGRESS].bitsAllocated, curTotalBits),
                   formatUsage(stats.gress[EGRESS].bitsAllocated, curTotalBits),
                   std::to_string(curTotalBits)});
    }

    tp.addSep();

    if (containerKinds.size() > 1) {
        // Print stats for usage by container kind.
        for (auto &kv : statsByContainerKind) {
            auto &kind = kv.first;
            auto &stats = kv.second;

            auto curTotalBits = totalBitsByKind.at(kind);
            auto totalContainers = totalContainersByKind.at(kind);

            std::stringstream ss;
            ss << "Usage for " << PHV::STR_OF_KIND.at(kind);

            tp.addRow({ss.str(), formatUsage(stats.total.containersUsed, totalContainers),
                       formatUsage(stats.total.bitsUsed, curTotalBits),
                       formatUsage(stats.gress[INGRESS].bitsUsed, curTotalBits),
                       formatUsage(stats.gress[EGRESS].bitsUsed, curTotalBits),
                       formatUsage(stats.total.bitsAllocated, curTotalBits),
                       formatUsage(stats.gress[INGRESS].bitsAllocated, curTotalBits),
                       formatUsage(stats.gress[EGRESS].bitsAllocated, curTotalBits),
                       std::to_string(curTotalBits)});
        }

        tp.addSep();
    }

    // Print stats for overall usage.
    tp.addRow({"Overall PHV Usage",
               formatUsage(overallStats.total.containersUsed, overallTotalContainers),
               formatUsage(overallStats.total.bitsUsed, overallTotalBits),
               formatUsage(overallStats.gress[INGRESS].bitsUsed, overallTotalBits),
               formatUsage(overallStats.gress[EGRESS].bitsUsed, overallTotalBits),
               formatUsage(overallStats.total.bitsAllocated, overallTotalBits),
               formatUsage(overallStats.gress[INGRESS].bitsAllocated, overallTotalBits),
               formatUsage(overallStats.gress[EGRESS].bitsAllocated, overallTotalBits),
               std::to_string(overallTotalBits)});

    tp.print();

    return ss.str();
}

cstring PHV::AllocationReport::printTagalongCollectionsOccupancyMetrics() const {
    const auto &phvSpec = Device::phvSpec();

    std::map<unsigned, TagalongCollectionInfo> tagalongCollectionInfos;

    auto tagalongCollectionSpec = phvSpec.getTagalongCollectionSpec();

    for (unsigned i = 0; i < phvSpec.getNumTagalongCollections(); i++) {
        auto &collectionInfo = tagalongCollectionInfos[i];

        for (auto &kv : tagalongCollectionSpec)
            collectionInfo.totalContainers[kv.first] = kv.second;
    }

    for (auto &kv : container_to_bits_used) {
        auto container = kv.first;
        auto bits_used_in_container = kv.second;
        auto bits_allocated_in_container = container_to_bits_allocated.at(container);
        auto gress = container_to_gress.at(container);

        if (!container.is(PHV::Kind::tagalong)) continue;

        if (bits_used_in_container == 0) continue;

        int collectionID = phvSpec.getTagalongCollectionId(container);
        auto &collectionInfo = tagalongCollectionInfos[collectionID];

        collectionInfo.containersUsed[container.type()]++;
        collectionInfo.bitsUsed[container.type()] += bits_used_in_container;
        collectionInfo.bitsAllocated[container.type()] += bits_allocated_in_container;

        if (collectionInfo.gress && gress) {
            BUG_CHECK(*(collectionInfo.gress) == *gress,
                      "Tagalong collection for container %1% assigned with fields from both "
                      "ingress and egress?",
                      container);
        }

        collectionInfo.gress = gress;
    }

    std::stringstream ss;
    ss << std::endl << "Tagalong Collections:" << std::endl;

    TablePrinter tp(ss, {"Collection", "Gress", "8b Containers Used", "16b Containers Used",
                         "32b Containers Used", "Bits Used", "Bits Allocated"});

    std::map<PHV::Type, int> totalUsed;
    std::map<PHV::Type, int> totalAvail;

    int totalUsedBits = 0, totalAllocatedBits = 0, totalAvailBits = 0;

    for (auto &kv : tagalongCollectionInfos) {
        auto &info = kv.second;

        tp.addRow({std::to_string(kv.first),
                   info.getTotalUsedBits() ? std::string(toSymbol(*(info.gress)).c_str()) : "",
                   info.printUsage(PHV::Type::TB), info.printUsage(PHV::Type::TH),
                   info.printUsage(PHV::Type::TW), info.printTotalUsage(), info.printTotalAlloc()});

        for (auto type : {PHV::Type::TB, PHV::Type::TH, PHV::Type::TW}) {
            totalUsed[type] += info.containersUsed[type];
            totalAvail[type] += info.totalContainers[type];
        }

        totalUsedBits += info.getTotalUsedBits();
        totalAllocatedBits += info.getTotalAllocatedBits();
        totalAvailBits += info.getTotalAvailableBits();
    }

    tp.addSep();
    tp.addRow({"Total", "", formatUsage(totalUsed[PHV::Type::TB], totalAvail[PHV::Type::TB]),
               formatUsage(totalUsed[PHV::Type::TH], totalAvail[PHV::Type::TH]),
               formatUsage(totalUsed[PHV::Type::TW], totalAvail[PHV::Type::TW]),
               formatUsage(totalUsedBits, totalAvailBits),
               formatUsage(totalAllocatedBits, totalAvailBits)});

    tp.print();

    return ss.str();
}

cstring PHV::AllocationReport::printOccupancyMetrics() const {
    std::stringstream ss;
    ss << std::endl << "PHV Allocation State" << std::endl;
    ss << printMauGroupsOccupancyMetrics();
    ss << std::endl;
    if (Device::currentDevice() == Device::TOFINO) ss << printTagalongCollectionsOccupancyMetrics();
    return ss.str();
}
