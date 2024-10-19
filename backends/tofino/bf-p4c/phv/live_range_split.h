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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_LIVE_RANGE_SPLIT_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_LIVE_RANGE_SPLIT_H_

#include <set>
#include <vector>

#include "phv/analysis/dark_live_range.h"
#include "phv/fieldslice_live_range.h"
#include "phv/phv_fields.h"

namespace PHV {

class AllocUtils;

/// Represents what bits of a container are used or available over all
/// possible stages.
class ContainerOccupancy {
    int minStage;
    int maxStage;
    /// Vector where each index represents a read or write substage of
    /// a stage. The bitvec at each index tracks the used bits of the
    /// container at the substage.
    std::vector<bitvec> vec;
    unsigned containerWidth;

    /// Convert a PHV::StageAndAccess into a index for \ref vec
    unsigned vecIdx(PHV::StageAndAccess sa) const {
        BUG_CHECK(sa.first >= minStage && sa.first <= maxStage,
                  "vecIdx (%1%) out of stage range [%2%, %3%]", sa.first, minStage, maxStage);
        int stage = sa.first - minStage;
        int offset = sa.second.isWrite() ? 1 : 0;
        int val = stage * 2 + offset;
        BUG_CHECK(val >= 0, "Invalid StageAndAccess stage %1%", val);
        return static_cast<unsigned>(val);
    }

    /// Finds sequences of bits that are available at a substage,
    /// given length and alignment constraints.
    ///
    /// \param i Index into the \ref vec vector, should be the output
    /// of vecIdx()
    /// \param minLength The minimum length of sequences to be returned
    /// \param alignment Alignment constraint, if not none, specifies
    /// that sequence start bits should be aligned to the given
    /// alignment
    ///
    /// \return List of (inclusive) bit ranges that are available
    std::vector<le_bitrange> availBitsAt(unsigned i, unsigned minLength);

    friend std::ostream &operator<<(std::ostream &, const ContainerOccupancy &);

 public:
    ContainerOccupancy() : containerWidth(0) {}

    ContainerOccupancy(int minStage, int maxStage, unsigned containerWidth)
        : minStage(minStage),
          maxStage(maxStage),
          vec((maxStage - minStage + 1) * 2),
          containerWidth(containerWidth) {
        BUG_CHECK(minStage >= -1, "ContainerOccupancy minStage (%1%) must be >= -1");
        BUG_CHECK(maxStage >= minStage, "Invalid ContainerOccupancy stage range (%1% > %2%)",
                  minStage, maxStage);
    }

    /// Set bits in range for stage span [begin, end]
    void setRange(StageAndAccess begin, StageAndAccess end, le_bitrange range);

    /// Find ranges of available bits in this container at the given
    /// StageAndAccess, with optional constraints on the bit length and
    /// alignment.
    std::vector<le_bitrange> availBitsAt(PHV::StageAndAccess sa, unsigned minLength = 0) {
        return availBitsAt(vecIdx(sa), minLength);
    }

    bool isRangeAvailable(StageAndAccess begin, StageAndAccess end, le_bitrange range);
};

/// Represents a span of free bits in a container over a range of
/// stages. Similar in representation to AllocSlice, but represents a
/// region that's available.
struct FreeSlice {
    Container container;
    le_bitrange range;
    LiveRange lr;

    bool operator==(const FreeSlice &c) const {
        return container == c.container && range == c.range && lr.start == c.lr.start &&
               lr.end == c.lr.end;
    }
    bool operator!=(const FreeSlice &c) const { return !(*this == c); }
    bool operator<(const FreeSlice &c) const {
        if (container != c.container) return container < c.container;
        if (range != c.range) return range < c.range;
        if (lr.start != c.lr.start) return lr.start < c.lr.start;
        if (lr.end != c.lr.end) return lr.end < c.lr.end;
        return false;
    }

    /// Returns true of the bit slice is fully available at the read
    /// and write substages of the given stage.
    bool isFullyAvailableAt(int stage) const {
        if (lr.start.first > stage || (lr.start.first == stage && !lr.start.second.isRead())) {
            return false;
        }
        if (lr.end.first < stage || (lr.end.first == stage && !lr.end.second.isWrite())) {
            return false;
        }
        return true;
    }

    friend std::ostream &operator<<(std::ostream &, const FreeSlice &);
};

/// Class for finding ways a live range can be split to fit into a
/// given Allocation.
class LiveRangeSplit {
 public:
    /// List of all sequences of containers that a live range can be
    /// split into, each with its full availabile range.
    using ContainerSequence = ordered_set<FreeSlice>;
    using ContainerSequences = std::set<ContainerSequence>;

 private:
    static const FieldUse READ;
    static const FieldUse WRITE;

    const Allocation &alloc;
    const PhvInfo &phvInfo;
    const AllocUtils &utils;
    const DependencyGraph &deps;
    const std::list<ContainerGroup *> containerGroups;
    const DarkLiveRangeMap &liveRangeMap;
    const NonMochaDarkFields &nonMochaDark;

    std::map<Container, ContainerGroup *> groupForContainer;
    std::map<const Field *, std::set<StageAndAccess>> nonMochaStagesForField;
    std::map<const Field *, std::set<StageAndAccess>> nonDarkStagesForField;

    std::set<StageAndAccess> getMochaIncompatibleStagesForField(const Field *field) const;
    std::set<StageAndAccess> getDarkIncompatibleStagesForField(const Field *field) const;

 public:
    explicit LiveRangeSplit(const Allocation &alloc, const PhvInfo &phv, const AllocUtils &utils,
                            const DependencyGraph &deps,
                            const std::list<ContainerGroup *> &containerGroups,
                            const DarkLiveRangeMap &liveRangeMap,
                            const NonMochaDarkFields &nonMochaDark);

    /// For each stage, find container (slices) that are available at each one.
    std::set<FreeSlice> buildContainerOccupancy(StageAndAccess begin, StageAndAccess end,
                                                const FieldSlice &fs) const;

    /// Builds a list of all possible allocations
    ContainerSequences buildContainerSeqs(const std::set<FreeSlice> &avail, StageAndAccess begin,
                                          StageAndAccess end) const;

    /// Find all the ways the given live range can be split into
    /// multiple containers, and returns all possible sequences of
    /// them.
    ContainerSequences findLiveRangeSplits(StageAndAccess begin, StageAndAccess end,
                                           const FieldSlice &fs) const;
    ContainerSequences findLiveRangeSplits(const LiveRangeInfo &lri, const FieldSlice &fs) const;

    static void report(const ContainerSequences &splits);
};

/// This pass takes a potentially incomplete allocation, and if there
/// are unallocated fields, analyzes the state of the PHV containers
/// for the fields that are allocated, and generates potential
/// allocatations for the unallocated ones. The new allocations will
/// be split amongst multiple containers if possible, specifying that
/// the field will have to move from one container to another where
/// their availability overlaps. If unsuccessful, compilation will
/// fail here.
///
/// TODO: This does not yet modify the allocation, instead only
/// outputting potential splits to a log file. So it will always fail
/// if \ref unallocated is not empty.
class LiveRangeSplitOrFail : public Visitor {
    std::function<const ConcreteAllocation *()> getAllocFn;
    const std::list<const PHV::SuperCluster *> &unallocated;
    PhvInfo &phv;
    const PHV::AllocUtils &utils;
    const DependencyGraph &deps;
    const DarkLiveRangeMap &liveRangeMap;
    const NonMochaDarkFields &nonMochaDark;

    const IR::Node *apply_visitor(const IR::Node *root, const char *name = 0) override;

 public:
    LiveRangeSplitOrFail(std::function<const ConcreteAllocation *()> getAllocFn,
                         const std::list<const PHV::SuperCluster *> &unallocated, PhvInfo &phv,
                         const PHV::AllocUtils &utils, const DependencyGraph &deps,
                         const DarkLiveRangeMap &liveRangeMap,
                         const NonMochaDarkFields &nonMochaDark)
        : getAllocFn(getAllocFn),
          unallocated(unallocated),
          phv(phv),
          utils(utils),
          deps(deps),
          liveRangeMap(liveRangeMap),
          nonMochaDark(nonMochaDark) {}
};

}  // namespace PHV

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_LIVE_RANGE_SPLIT_H_ */
