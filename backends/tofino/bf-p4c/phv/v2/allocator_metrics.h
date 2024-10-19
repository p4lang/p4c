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

#ifndef BF_P4C_PHV_V2_ALLOCATOR_METRICS_H_
#define BF_P4C_PHV_V2_ALLOCATOR_METRICS_H_

#include <chrono>

#include "bf-p4c/phv/phv.h"

namespace PHV {
namespace v2 {

/// AllocatorMetrics contains metrics useful in tracking Allocator efficiency and debug
class AllocatorMetrics {
 public:
    enum ctype { ALLOC = 0, EQUIV = 1 };

 private:
    cstring name;

    // Allocation
    // PHV::Type (Kind, Size) -> pair < total, pass >
    typedef std::map<PHV::Type, std::pair<unsigned long, unsigned long>> ContainerMetricsMap;
    ContainerMetricsMap alloc_containers;
    ContainerMetricsMap equiv_containers;

    // Compilation Time
    std::chrono::time_point<std::chrono::steady_clock> start_time;
    std::chrono::time_point<std::chrono::steady_clock> stop_time;

 public:
    explicit AllocatorMetrics(cstring name = ""_cs) : name(name) {}

    cstring get_name() const { return name; };

    // Container Equivalence
    void update_container_equivalence_metrics(const PHV::Container &c) {
        ++equiv_containers[c.type()].first;
    }

    // Allocation
    void update_containers_metrics(const PHV::Container &c, bool succ = false) {
        ++alloc_containers[c.type()].first;
        if (succ) ++alloc_containers[c.type()].second;
    }

    unsigned long get_containers(const ctype ct = ALLOC, const PHV::Kind *kind = nullptr,
                                 const PHV::Size *size = nullptr, bool succ = false) const {
        unsigned long containers = 0;

        const ContainerMetricsMap *cmap = nullptr;
        if (ct == ALLOC)
            cmap = &alloc_containers;
        else if (ct == EQUIV)
            cmap = &equiv_containers;
        else
            return 0;

        for (auto ct : *cmap) {
            if (kind && (ct.first.kind() != *kind)) continue;
            if (size && (ct.first.size() != *size)) continue;
            if (succ)
                containers += ct.second.second;
            else
                containers += ct.second.first;
        }
        return containers;
    }

    // Compilation time
    void start_clock() { start_time = std::chrono::steady_clock::now(); }

    void stop_clock() { stop_time = std::chrono::steady_clock::now(); }

    std::string get_duration() const {
        std::stringstream ss;
        const auto duration = stop_time - start_time;
        const auto hrs = std::chrono::duration_cast<std::chrono::hours>(duration);
        const auto mins = std::chrono::duration_cast<std::chrono::minutes>(duration - hrs);
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration - hrs - mins);
        const auto msecs =
            std::chrono::duration_cast<std::chrono::milliseconds>(duration - hrs - mins - secs);

        ss << hrs.count() << "h " << mins.count() << "m " << secs.count() << "s " << msecs.count()
           << "ms " << std::endl;
        return ss.str();
    }

    void clear() {
        alloc_containers.clear();
        equiv_containers.clear();
    }
};

std::ostream &operator<<(std::ostream &out, const AllocatorMetrics &am);

}  // namespace v2
}  // namespace PHV

#endif /* BF_P4C_PHV_V2_ALLOCATOR_METRICS_H_ */
