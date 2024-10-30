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

#ifndef BF_P4C_PHV_UTILS_REPORT_H_
#define BF_P4C_PHV_UTILS_REPORT_H_

#include "bf-p4c/phv/utils/utils.h"

namespace PHV {

class AllocationReport {
    const PhvUse &uses;
    const Allocation &alloc;
    bool printMetricsOnly = false;

    ordered_map<std::optional<gress_t>,
                ordered_map<PHV::Type, ordered_map<Allocation::ContainerAllocStatus, int>>>
        alloc_status;

    ordered_map<PHV::Container, int> partial_containers_stat;

    ordered_map<PHV::Container, int> container_to_bits_used;

    ordered_map<PHV::Container, int> container_to_bits_allocated;

    ordered_map<PHV::Container, std::optional<gress_t>> container_to_gress;

    ordered_set<AllocSlice> alloc_slices;

    int total_allocated_bits = 0;
    int total_unallocated_bits = 0;
    int valid_ingress_unallocated_bits = 0;
    int valid_egress_unallocated_bits = 0;

    static std::string formatPercent(int div, int den) {
        std::stringstream ss;
        ss << boost::format("(%=6.3g%%)") % (100.0 * div / den);
        return ss.str();
    }

    static std::string formatUsage(int used, int total, bool show_total = false) {
        std::stringstream ss;
        ss << used << " ";
        if (show_total) ss << "/ " << total << " ";
        ss << formatPercent(used, total);
        return ss.str();
    }

    struct PhvOccupancyMetricFields {
        unsigned containersUsed = 0;
        unsigned bitsUsed = 0;
        unsigned bitsAllocated = 0;

        inline PhvOccupancyMetricFields &operator+=(const PhvOccupancyMetricFields &src) {
            containersUsed += src.containersUsed;
            bitsUsed += src.bitsUsed;
            bitsAllocated += src.bitsAllocated;

            return *this;
        }
    };

    /// Information about a set of PHV containers.
    struct PhvOccupancyMetric {
        // in total
        PhvOccupancyMetricFields total;
        // per gress
        std::map<gress_t, PhvOccupancyMetricFields> gress;

        inline PhvOccupancyMetric &operator+=(const PhvOccupancyMetric &src) {
            total += src.total;
            for (const auto &gr : src.gress) {
                gress[gr.first] += gr.second;
            }

            return *this;
        }

        inline std::string gressToSymbolString() const {
            std::stringstream ss;
            std::string sep = "";
            for (const auto &gr : gress) {
                ss << sep;
                ss << toSymbol(gr.first);
                sep = "/";
            }
            return ss.str();
        }
    };

    /// Information about PHV containers in each MAU group
    struct MauGroupInfo {
        size_t size = 0;
        int groupID = 0;

        PhvOccupancyMetric totalStats;
        std::map<PHV::Kind, PhvOccupancyMetric, std::greater<PHV::Kind>> statsByContainerKind;

        MauGroupInfo() {}

        /// Initializes a new MauGroupInfo with usage information from a single container.
        explicit MauGroupInfo(size_t sz, int i, PHV::Kind containerKind, bool cont, size_t used,
                              size_t allocated, std::optional<gress_t> gr)
            : size(sz), groupID(i) {
            update(containerKind, cont, used, allocated, gr);
        }

        /// Adds usage information from a single container
        void update(PHV::Kind containerKind, bool cont, size_t used, size_t allocated,
                    std::optional<gress_t> gr) {
            auto &kindStats = statsByContainerKind[containerKind];

            // Update total stats
            auto &totalStatsTotal = totalStats.total;
            auto &kindStatsTotal = kindStats.total;

            if (cont) {
                ++totalStatsTotal.containersUsed;
                ++kindStatsTotal.containersUsed;
            }

            totalStatsTotal.bitsUsed += used;
            kindStatsTotal.bitsUsed += used;

            totalStatsTotal.bitsAllocated += allocated;
            kindStatsTotal.bitsAllocated += allocated;

            if (gr) {
                // Update per gress stats
                auto &totalStatsGress = totalStats.gress[*gr];
                auto &kindStatsGress = kindStats.gress[*gr];

                if (cont) {
                    ++totalStatsGress.containersUsed;
                    ++kindStatsGress.containersUsed;
                }

                totalStatsGress.bitsUsed += used;
                kindStatsGress.bitsUsed += used;

                totalStatsGress.bitsAllocated += allocated;
                kindStatsGress.bitsAllocated += allocated;
            }
        }
    };

    /// Information about T-PHV containers in each collection
    struct TagalongCollectionInfo {
        std::map<PHV::Type, size_t> containersUsed;
        std::map<PHV::Type, size_t> bitsUsed;
        std::map<PHV::Type, size_t> bitsAllocated;
        std::map<PHV::Type, size_t> totalContainers;

        std::optional<gress_t> gress;

        size_t getTotalUsedBits() {
            size_t rv = 0;
            for (auto kv : bitsUsed) rv += kv.second;
            return rv;
        }

        size_t getTotalAllocatedBits() {
            size_t rv = 0;
            for (auto kv : bitsAllocated) rv += kv.second;
            return rv;
        }

        size_t getTotalAvailableBits() {
            size_t rv = 0;
            for (auto &kv : totalContainers) rv += (size_t)kv.first.size() * kv.second;
            return rv;
        }

        std::string printUsage(PHV::Type type) {
            auto used = containersUsed[type];
            auto total = totalContainers[type];

            return formatUsage(used, total);
        }

        std::string printTotalUsage() {
            auto total = getTotalAvailableBits();
            auto used = getTotalUsedBits();

            return formatUsage(used, total);
        }

        std::string printTotalAlloc() {
            auto total = getTotalAvailableBits();
            auto alloced = getTotalAllocatedBits();

            return formatUsage(alloced, total);
        }
    };

 public:
    explicit AllocationReport(const Allocation &alloc, bool printMetricsOnly = false)
        : uses(*(alloc.uses_i)), alloc(alloc), printMetricsOnly(printMetricsOnly) {}

    /// @returns a summary of the status of each container by type and gress.
    cstring printSummary() {
        collectStatus();

        std::stringstream ss;

        if (!printMetricsOnly) ss << printAllocation() << std::endl;

        if (!printMetricsOnly) ss << printAllocSlices() << std::endl;

        ss << printContainerStatus() << std::endl;
        ss << printOverlayStatus() << std::endl;
        ss << printOccupancyMetrics() << std::endl;

        return ss.str();
    }

 private:
    void collectStatus();

    cstring printAllocation() const;
    cstring printAllocSlices() const;
    cstring printOverlayStatus() const;
    cstring printContainerStatus();
    cstring printOccupancyMetrics() const;
    cstring printMauGroupsOccupancyMetrics() const;
    cstring printTagalongCollectionsOccupancyMetrics() const;
};

}  // namespace PHV

#endif /* BF_P4C_PHV_UTILS_REPORT_H_ */
