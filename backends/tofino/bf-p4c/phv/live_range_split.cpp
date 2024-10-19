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

#include "live_range_split.h"

#include "bf-p4c/device.h"
#include "bf-p4c/phv/allocate_phv.h"
#include "bf-p4c/phv/fieldslice_live_range.h"
#include "bf-p4c/phv/utils/report.h"

namespace PHV {

const FieldUse LiveRangeSplit::READ = FieldUse(PHV::FieldUse::READ);
const FieldUse LiveRangeSplit::WRITE = FieldUse(PHV::FieldUse::WRITE);

std::vector<le_bitrange> ContainerOccupancy::availBitsAt(unsigned i, unsigned minLength) {
    BUG_CHECK(containerWidth != 0, "uninitialized ContainerOccupancy");
    BUG_CHECK(minLength > 0, "minLength (%1%) must be at least 1", minLength);
    std::vector<le_bitrange> result;
    bitvec bv = vec[i];
    unsigned start = 0;

    // Algorithm: Find first 0 in occupancy bitvec, then the first
    // following 1. Add sequence to result if valid.
    while (true) {
        start = bv.ffz(start);
        if (start >= containerWidth) {
            // Reached end of container.
            break;
        }
        unsigned end = bv.ffs(start);
        if (end >= containerWidth) {
            // Reached end of container, but have a sequence of 0's,
            // so add it to the result.
            if (containerWidth - start >= minLength) {
                unsigned lastBit = containerWidth;
                result.push_back(le_bitrange(FromTo(start, lastBit - 1)));
            }
            break;
        }
        if (end - start >= minLength) {
            result.push_back(le_bitrange(FromTo(start, end - 1)));
        }
        start = end;
    }
    return result;
}

void ContainerOccupancy::setRange(StageAndAccess begin, StageAndAccess end, le_bitrange range) {
    BUG_CHECK(containerWidth != 0, "uninitialized ContainerOccupancy");
    BUG_CHECK(end >= begin, "setRange begin (%1%) > end (%2%)", begin, end);
    if (begin.first > maxStage || end.first < minStage) {
        return;
    }

    // Clamp to size
    if (begin.first < minStage) {
        begin = StageAndAccess{minStage, FieldUse(FieldUse::READ)};
    }
    if (end.first > maxStage) {
        end = StageAndAccess{maxStage, FieldUse(FieldUse::WRITE)};
    }

    unsigned beginIdx = vecIdx(begin);
    unsigned endIdx = vecIdx(end);

    for (unsigned i = beginIdx; i <= endIdx; i++) {
        vec[i].setrange(range.lo, range.hi - range.lo + 1);
    }
}

bool ContainerOccupancy::isRangeAvailable(StageAndAccess begin, StageAndAccess end,
                                          le_bitrange range) {
    BUG_CHECK(containerWidth != 0, "uninitialized ContainerOccupancy");
    unsigned beginIdx = vecIdx(begin);
    unsigned endIdx = vecIdx(end);
    BUG_CHECK(beginIdx < vec.size() && endIdx < vec.size(),
              "Invalid slice indices: beginIdx (%1%) and endIdx (%2%) must be < vec size (%3%)",
              beginIdx, endIdx, vec.size());

    for (unsigned i = beginIdx; i <= endIdx; i++) {
        bitvec slice = vec[i].getslice(range.lo, range.hi - range.lo + 1);
        if (!slice.empty()) {
            return false;
        }
    }

    return true;
}

std::ostream &operator<<(std::ostream &os, const ContainerOccupancy &occ) {
    if (occ.vec.empty()) {
        return os;
    }

    // Header for bit labels
    if (occ.containerWidth > 10) {
        os << "      ";
        for (unsigned i = 0; i < occ.containerWidth; i++) {
            os << (i / 10);
        }
        os << '\n';
    }
    os << "      ";
    for (unsigned i = 0; i < occ.containerWidth; i++) {
        os << (i % 10);
    }
    os << '\n';
    os << "      " << std::setw(occ.containerWidth + 1) << std::setfill('-') << '\n'
       << std::setfill(' ');

    // Table of occupancy for each stage
    for (int stage = occ.minStage; stage <= occ.maxStage; stage++) {
        for (int use : {FieldUse::READ, FieldUse::WRITE}) {
            auto stgacc = StageAndAccess{stage, FieldUse(use)};
            os << std::setw(3) << stgacc << ": ";
            unsigned vecIndex = occ.vecIdx(stgacc);
            BUG_CHECK(vecIndex < occ.vec.size(), "vecIndex out of range (%1% >= %2%", vecIndex,
                      occ.vec.size());
            for (unsigned bit = 0; bit < occ.containerWidth; bit++) {
                os << (occ.vec[vecIndex][bit] ? 'X' : '.');
            }
            os << '\n';
        }
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const FreeSlice &cs) {
    os << cs.container << '.' << cs.range << " {" << cs.lr.start << ", " << cs.lr.end << '}';
    return os;
}

LiveRangeSplit::LiveRangeSplit(const Allocation &alloc, const PhvInfo &phv, const AllocUtils &utils,
                               const DependencyGraph &deps,
                               const std::list<ContainerGroup *> &containerGroups,
                               const DarkLiveRangeMap &liveRangeMap,
                               const NonMochaDarkFields &nonMochaDark)
    : alloc(alloc),
      phvInfo(phv),
      utils(utils),
      deps(deps),
      containerGroups(containerGroups),
      liveRangeMap(liveRangeMap),
      nonMochaDark(nonMochaDark) {
    for (auto *group : containerGroups) {
        for (auto c : *group) {
            groupForContainer[c] = group;
        }
    }

    auto dumpNonMochaDarkFields = [&](const NonMochaDarkFields::FieldMap &fieldMap,
                                      std::map<const Field *, std::set<StageAndAccess>> &stageMap) {
        for (const auto &kv : fieldMap) {
            const Field *field = phv.field(kv.first);
            const auto &tablesAndUses = kv.second;
            for (const auto &tableAndUse : tablesAndUses) {
                const IR::MAU::Table *table = tableAndUse.first;
                const FieldUse use = tableAndUse.second;
                std::set<int> tblStages = phvInfo.minStages(table);
                BUG_CHECK(!tblStages.empty(), "table has no minStages");
                int stage = *tblStages.begin();
                auto stgacc = StageAndAccess{stage, use};
                stageMap[field].insert(stgacc);
                LOG_DEBUG2(TAB1 << *field << " at " << stgacc);
            }
        }
    };

    LOG_DEBUG2("Non-mocha accesses:");
    dumpNonMochaDarkFields(nonMochaDark.getNonMocha(), nonMochaStagesForField);

    LOG_DEBUG2("\nNon-dark accesses:");
    dumpNonMochaDarkFields(nonMochaDark.getNonDark(), nonDarkStagesForField);
}

std::set<StageAndAccess> LiveRangeSplit::getMochaIncompatibleStagesForField(
    const Field *field) const {
    std::set<StageAndAccess> result;
    auto stages = nonMochaStagesForField.find(field);
    if (stages != nonMochaStagesForField.end()) {
        result.insert(stages->second.begin(), stages->second.end());
    }
    return result;
}

std::set<StageAndAccess> LiveRangeSplit::getDarkIncompatibleStagesForField(
    const Field *field) const {
    std::set<StageAndAccess> result;
    auto stages = nonDarkStagesForField.find(field);
    if (stages != nonDarkStagesForField.end()) {
        result.insert(stages->second.begin(), stages->second.end());
    }
    return result;
}

std::set<FreeSlice> LiveRangeSplit::buildContainerOccupancy(StageAndAccess begin,
                                                            StageAndAccess end,
                                                            const FieldSlice &fs) const {
    std::set<FreeSlice> result;

    LOG_DEBUG2("Building container availability for stages " << begin << ':' << end
                                                             << " and FieldSlice " << fs);

    for (auto cid : Device::phvSpec().physicalContainers()) {
        Container container = Device::phvSpec().idToContainer(cid);
        auto kind = container.type().kind();
        if (kind == Kind::tagalong) {
            // Tagalong containers are not available for splitting into.
            continue;
        }

        if (static_cast<int>(container.size()) < fs.size()) {
            // Container is smaller than field slice that needs to be allocated.
            LOG_DEBUG3("- " << container << " is smaller than the field slice");
            continue;
        }

        auto allocSlices = alloc.slices(container);
        if (!allocSlices.empty() && allocSlices.begin()->field()->gress != fs.field()->gress) {
            // This container is being used for a different gress and
            // is unavailable to this field.
            LOG_DEBUG3("- " << container << " is being used by different gress");
            continue;
        }

        int containerStartStage = begin.first;
        int containerEndStage = end.first;
        if (kind == Kind::dark) {
            if (begin.first == PhvInfo::getDeparserStage() || end.first == -1) {
                // Range is only parser or deparser stage, no dark containers will work.
                LOG_DEBUG3("- Dark container " << container
                                               << " cannot be used in parser/deparser only");
                continue;
            }
            containerStartStage = std::max(containerStartStage, 0);
            containerEndStage = std::min(containerEndStage, PhvInfo::getDeparserStage() - 1);
        }
        ContainerOccupancy occupancy(containerStartStage, containerEndStage, container.size());
        std::optional<gress_t> containerGress;
        const Field *solitaryField = nullptr;

        // For each allocated slice in the container, find what stages
        // it is live at and mark it in the ContainerOccupancy
        // object.
        for (const auto &slice : allocSlices) {
            // Record the gress of allocated fields.
            containerGress = slice.field()->gress;

            if (slice.field()->is_solitary()) {
                // Containers occupied by a solitary field are not candidates
                // for a split field.
                solitaryField = slice.field();
                break;
            }

            // TODO better/more fine grained mutex analysis
            if (phvInfo.isFieldMutex(fs.field(), slice.field())) {
                continue;
            }

            // Writes to mocha/dark set all bits in the
            // container regardless of slice size, so mark all
            // bits as in use for the slice's live range.
            auto bitsToSet = kind == Kind::mocha || kind == Kind::dark
                                 ? le_bitrange(0, container.size() - 1)
                                 : slice.container_slice();

            auto earliestAccess = liveRangeMap.getEarliestAccess(slice.field());
            auto latestAccess = liveRangeMap.getLatestAccess(slice.field());
            auto liveRangeBegin = earliestAccess ? *earliestAccess : slice.getEarliestLiveness();
            auto liveRangeEnd = latestAccess ? *latestAccess : slice.getLatestLiveness();
            LOG_DEBUG3("setRange " << bitsToSet << " from " << liveRangeBegin << " to "
                                   << liveRangeEnd << " for " << slice);
            occupancy.setRange(liveRangeBegin, liveRangeEnd, bitsToSet);

            // Containers written to in parser stage can only have one
            // field, or one field per byte in Tofino 2+.
            if (slice.field()->parsed()) {
                auto parserBitsToSet = [&]() {
                    if (Device::currentDevice() == Device::TOFINO) {
                        // Set all bits of container.
                        return le_bitrange(0, container.size() - 1);
                    }
                    // For Tofino 2+, set just the whole bytes
                    // occupied by the slice.
                    auto cs = slice.container_slice();
                    unsigned startByte = cs.lo / 8;
                    unsigned endByte = cs.hi / 8;
                    return le_bitrange(startByte * 8, endByte * 8 + 7);
                }();
                LOG_DEBUG4("\tsetting bits [" << parserBitsToSet.lo << ", " << parserBitsToSet.hi
                                              << "] at parser stage for field " << slice);
                occupancy.setRange(StageAndAccess{-1, READ}, StageAndAccess{-1, WRITE},
                                   parserBitsToSet);
            }
        }

        if (solitaryField) {
            LOG_DEBUG3("- " << container << " is solitary occupied by " << *solitaryField);
            continue;
        }

        if (kind == Kind::mocha) {
            auto stagesAndAccesses = getMochaIncompatibleStagesForField(fs.field());
            for (const auto &stgacc : stagesAndAccesses) {
                if (stgacc.first < begin.first || stgacc.first > end.first) {
                    continue;
                }
                LOG_DEBUG3("container " << container
                                        << ": setRange for mocha incompatible access: " << stgacc);
                occupancy.setRange(stgacc, stgacc, le_bitrange(0, container.size() - 1));
            }
        }
        if (kind == Kind::dark) {
            auto stagesAndAccesses = getDarkIncompatibleStagesForField(fs.field());
            for (const auto &stgacc : stagesAndAccesses) {
                if (stgacc.first < begin.first || stgacc.first > end.first) {
                    continue;
                }
                LOG_DEBUG3("container " << container
                                        << ": setRange for dark incompatible access: " << stgacc);
                occupancy.setRange(stgacc, stgacc, le_bitrange(0, container.size() - 1));
            }
            // Set 0r substage, nothing can split into it since it is
            // the first substage.
            LOG_DEBUG3("container " << container << ": set 0r bits");
            auto stage0r = StageAndAccess{0, READ};
            occupancy.setRange(stage0r, stage0r, le_bitrange(0, container.size() - 1));
        }

        if (liveRangeMap.count(fs.field()) && fs.alignment()) {
            auto entry = liveRangeMap.at(fs.field());
            for (const auto &stgacc : Keys(entry)) {
                for (size_t byte = 0; byte < container.size() / 8; byte++) {
                    if (!fs.alignment()->isByteAligned()) {
                        le_bitrange range(byte * 8, byte * 8 + fs.alignment()->align - 1);
                        LOG_DEBUG3("container " << container << ": set " << range
                                                << " before aligned field");
                        occupancy.setRange(stgacc, stgacc, range);
                    }
                    if (fs.size() + fs.alignment()->align != 8) {
                        le_bitrange range(byte * 8 + fs.size() + fs.alignment()->align,
                                          byte * 8 + 7);
                        LOG_DEBUG3("container " << container << ": set bits " << range
                                                << " after aligned field");
                        occupancy.setRange(stgacc, stgacc, range);
                    }
                }
            }
        }

        LOG_DEBUG2("- " << container << " container availability:");
        LOG_DEBUG2(occupancy);

        // Iterate through all stages between startStage and
        // endStage. If a range of bits is available at that stage,
        // keep track of it in activeRanges. Once a range in there
        // stops being available, it's full stage availability is
        // known and it is added to the result.
        std::vector<FreeSlice> activeRanges;

        for (int stage = containerStartStage; stage <= containerEndStage; stage++) {
            for (auto use : {FieldUse::READ, FieldUse::WRITE}) {
                auto stgacc = StageAndAccess{stage, FieldUse(use)};
                auto ranges = occupancy.availBitsAt(stgacc, fs.size());
                for (auto iter = activeRanges.begin(); iter != activeRanges.end();) {
                    FreeSlice &activeRange = *iter;
                    auto rangeOverlaps = [&](le_bitrange range) {
                        return activeRange.range.contains(range) ||
                               range.contains(activeRange.range);
                    };
                    auto matchingRange = std::find_if(ranges.begin(), ranges.end(), rangeOverlaps);
                    if (matchingRange == ranges.end()) {
                        // Reached end of this FreeSlice's live
                        // range, so add it to the result.
                        LOG_DEBUG3("\t\tInsert available range to result: " << activeRange);
                        result.insert(activeRange);
                        LOG_DEBUG4("\t\tErase active range: " << *iter);
                        iter = activeRanges.erase(iter);
                    } else {
                        // Extend the range to the current stage.
                        auto newEnd = StageAndAccess{stage, FieldUse(use)};
                        le_bitrange newRange(std::max(activeRange.range.lo, matchingRange->lo),
                                             std::min(activeRange.range.hi, matchingRange->hi));
                        if (activeRange.range != newRange) {
                            LOG_DEBUG4("\t\tUpdate range from " << iter->range << " to "
                                                                << newRange);
                            activeRange.range = newRange;
                        }
                        LOG_DEBUG4("\t\tUpdate end of " << *iter << " to " << newEnd);
                        activeRange.lr.end = newEnd;
                        ranges.erase(matchingRange);
                        ++iter;
                    }
                }

                // Update current activeRanges to have their liveness
                // end be at the current stage here.
                // Add new bit ranges that were not found in
                // activeRanges to activeRanges here.
                for (const auto &range : ranges) {
                    auto newSlice = FreeSlice{container, range, LiveRange{stgacc, stgacc}};
                    LOG_DEBUG4("\t\tAdd new active range: " << newSlice);
                    activeRanges.push_back(newSlice);
                }

                if (activeRanges.empty()) {
                    LOG_DEBUG4("\tNo active ranges at " << stgacc);
                } else {
                    std::ostringstream os;
                    for (size_t i = 0, e = activeRanges.size(); i < e; i++) {
                        os << activeRanges[i];
                        if (i < e - 1) {
                            os << ", ";
                        }
                    }
                    LOG_DEBUG4("\t" << stgacc << " active ranges: " << os.str());
                }
            }
        }
        // Any remaining active ranges should be added to the result
        // as they go to the last stage.
        for (const auto &activeRange : activeRanges) {
            LOG_DEBUG3("\t\tInsert remaining available range: " << activeRange);
            result.insert(activeRange);
        }
    }

    // Remove containers that are only available in one stage.
    // TODO this can be done with std::erase_if when we have c++20
    for (auto cs = result.begin(), end = result.end(); cs != end;) {
        if (cs->lr.start == cs->lr.end) {
            LOG_DEBUG3("\tErase single-stage container: " << *cs);
            cs = result.erase(cs);
        } else {
            ++cs;
        }
    }

    return result;
}

LiveRangeSplit::ContainerSequences LiveRangeSplit::buildContainerSeqs(
    const std::set<FreeSlice> &avail, StageAndAccess begin, StageAndAccess end) const {
    LiveRangeSplit::ContainerSequences result, partialResult;

    // Gather FreeSlices into splits. Each split has containers
    // from the same container group and with at least one stage fully
    // available between begin and end.

    std::map<ContainerGroup *, LiveRangeSplit::ContainerSequence> groupToSplitMap;
    LiveRange liverange = {begin, end};

    // Go through available container slices, and add ones that
    // intersect with the live range to map.
    for (const auto &containerSlice : avail) {
        if ((containerSlice.lr.end < liverange.start) ||
            (containerSlice.lr.start > liverange.end)) {
            LOG_DEBUG3("container slice " << containerSlice << " is disjoint with " << liverange)
            continue;
        }
        BUG_CHECK(groupForContainer.count(containerSlice.container),
                  "container not in groupForContainer");
        ContainerGroup *group = groupForContainer.at(containerSlice.container);
        LOG_DEBUG2("Adding to groupToSplitMap: " << *group << " -> " << containerSlice);
        groupToSplitMap[group].insert(containerSlice);
    }

    // Prune out splits that don't cover the full range.
    for (const auto &split : Values(groupToSplitMap)) {
        // Track the ability for containers to be available at each
        // stage. Splits (moving from one container to another) can
        // only happen between the read substage and write substage of
        // the same stage. So a container can handle the read substage
        // if it is available there, along with the previous write
        // substage. Similarly, it can handle the write substage if it
        // is also available in the following read substage.

        // stageUseMap maps stages to substages, marking a stage ->
        // substage as R/W if a container can handle that point. If at
        // the end all stages are present and RW, this split is vaild
        // and added to result.
        std::map<int, FieldUse> stageUseMap;

        for (const auto &cs : split) {
            int startStage = std::max(cs.lr.start.first, begin.first);
            int endStage = std::min(cs.lr.end.first, end.first);
            for (int stage = startStage; stage <= endStage; stage++) {
                auto prevWrite = StageAndAccess{stage - 1, WRITE};
                auto nextRead = StageAndAccess{stage + 1, READ};

                if (stage == startStage) {
                    // At start stage, container doesn't have W of prev stage.
                    // If R at this stage -> mark R
                    if (cs.lr.start.second.isRead()) {
                        stageUseMap[stage] |= READ;
                    }
                    // If W at this stage and R at next -> mark W
                    if (cs.lr.start.second.isWrite() && nextRead <= cs.lr.end) {
                        stageUseMap[stage] |= WRITE;
                    }
                } else if (stage == endStage) {
                    // At end stage, container doesn't have R of next stage.
                    // If R at this stage and W at prev -> mark R
                    if (cs.lr.end.second.isWrite() && prevWrite >= cs.lr.start) {
                        stageUseMap[stage] |= READ;
                    }
                    // If W at this stage -> mark W
                    if (cs.lr.end.second.isRead()) {
                        stageUseMap[stage] |= WRITE;
                    }
                } else {
                    // In middle, container has R and W this stage.
                    // If W at prev -> mark R
                    if (prevWrite >= cs.lr.start) {
                        stageUseMap[stage] |= READ;
                    }
                    // If R at next -> mark W
                    if (nextRead <= cs.lr.end) {
                        stageUseMap[stage] |= WRITE;
                    }
                }

                if (LOGGING(4)) {
                    LOG_DEBUG4("at end of scanning " << stage << " for " << cs
                                                     << ", stage use map:");
                    for (const auto &kv : stageUseMap) {
                        LOG_DEBUG4("\t" << kv.first << " -> " << kv.second);
                    }
                }
            }
        }

        // If stageUseMap covers all stages and all entries are RW,
        // add to full result. Otherwise add to partialResult, which
        // will only be returned if there are no entries in result.
        if (stageUseMap.size() == static_cast<unsigned>(end.first - begin.first + 1) &&
            std::all_of(
                stageUseMap.begin(), stageUseMap.end(),
                [](const std::pair<int, FieldUse> &kv) { return kv.second.isReadWrite(); })) {
            result.insert(split);
        } else {
            partialResult.insert(split);
        }
    }

    return result.empty() ? partialResult : result;
}

LiveRangeSplit::ContainerSequences LiveRangeSplit::findLiveRangeSplits(StageAndAccess begin,
                                                                       StageAndAccess end,
                                                                       const FieldSlice &fs) const {
    BUG_CHECK(fs.field(), "No field in live range info");

    std::set<FreeSlice> avail = buildContainerOccupancy(
        StageAndAccess{-1, READ}, StageAndAccess{PhvInfo::getDeparserStage(), WRITE}, fs);
    LOG_DEBUG1("\nThere are " << avail.size() << " available spots between " << begin << " and "
                              << end << " for " << fs);
    for (const auto &slice : avail) {
        LOG_DEBUG1('\t' << slice.container << '.' << slice.range << " {" << slice.lr.start << ", "
                        << slice.lr.end << '}');
    }

    ContainerSequences seqs = buildContainerSeqs(avail, begin, end);

    return seqs;
}

LiveRangeSplit::ContainerSequences LiveRangeSplit::findLiveRangeSplits(const LiveRangeInfo &lri,
                                                                       const FieldSlice &fs) const {
    // Get StageAndAccess for live range start and end.
    std::optional<int> startStage = std::nullopt;
    std::optional<int> endStage = std::nullopt;
    LiveRangeInfo::OpInfo startOp = LiveRangeInfo::OpInfo::DEAD;
    LiveRangeInfo::OpInfo endOp = LiveRangeInfo::OpInfo::DEAD;

    for (int i = 0; i < static_cast<int>(lri.vec().size()); i++) {
        if (!startStage && lri.stage(i) != LiveRangeInfo::OpInfo::DEAD) {
            *startStage = i;
            startOp = lri.stage(i);
        } else if (startStage && lri.stage(i) == LiveRangeInfo::OpInfo::DEAD) {
            *endStage = i;
            endOp = lri.stage(i);
            break;
        }
    }
    BUG_CHECK(startStage, "no startStage");
    BUG_CHECK(endStage, "no endStage");
    BUG_CHECK(*endStage >= *startStage, "startStage (%1%) > endStage (%2%)", *startStage,
              *endStage);
    auto isRW = [](LiveRangeInfo::OpInfo oi) {
        return oi == LiveRangeInfo::OpInfo::READ || oi == LiveRangeInfo::OpInfo::WRITE ||
               oi == LiveRangeInfo::OpInfo::READ_WRITE;
    };
    BUG_CHECK(isRW(startOp), "Invalid live range start");
    BUG_CHECK(isRW(endOp), "Invalid live range end");

    auto toFieldUse = [](LiveRangeInfo::OpInfo oi) {
        switch (oi) {
            case LiveRangeInfo::OpInfo::READ:
                return FieldUse::READ;
            case LiveRangeInfo::OpInfo::WRITE:
                return FieldUse::WRITE;
            case LiveRangeInfo::OpInfo::READ_WRITE:
                return FieldUse::READWRITE;
            default:
                BUG("invalid OpInfo");
                return FieldUse::LIVE;
        }
    };

    StageAndAccess start{*startStage, toFieldUse(startOp)};
    StageAndAccess end{*endStage, toFieldUse(endOp)};

    return findLiveRangeSplits(start, end, fs);
}

void LiveRangeSplit::report(const ContainerSequences &splits) {
    if (!LOGGING(1)) {
        return;
    }

    unsigned i = 0;
    for (const auto &seq : splits) {
        LOG_DEBUG1("\tsplit " << i++);
        for (const auto &cs : seq) {
            LOG_DEBUG1("\t\t" << cs.container << "[" << cs.range.lo << ":" << cs.range.hi << "] {"
                              << cs.lr.start << ", " << cs.lr.end << "}");
        }
    }
    LOG_DEBUG1('\n');
}

const IR::Node *LiveRangeSplitOrFail::apply_visitor(const IR::Node *root, const char *) {
    if (unallocated.empty()) {
        return root;
    }

    const ConcreteAllocation *alloc = getAllocFn();

    const int pipeId = root->to<IR::BFN::Pipe>()->canon_id();
    auto lrsFilename = Logging::PassManager::getNewLogFileName("live_range_split_"_cs);
    auto lrsLogfile = new Logging::FileLog(pipeId, lrsFilename, Logging::Mode::AUTO);

    PHV::LiveRangeSplit lrs(*alloc, phv, utils, deps,
                            PHV::AllocUtils::make_device_container_groups(), liveRangeMap,
                            nonMochaDark);
    for (auto *sc : unallocated) {
        sc->forall_fieldslices([&](const PHV::FieldSlice &s) {
            if (const PHV::Field *field = s.field()) {
                if (!liveRangeMap.count(field)) {
                    LOG_DEBUG1("Field " << *field << " not in DarkLiveRangeMap");
                    return;
                }
                auto begin = liveRangeMap.getEarliestAccess(field);
                BUG_CHECK(begin, "field earliest access not in DarkLiveRangeMap");
                auto end = liveRangeMap.getLatestAccess(field);
                BUG_CHECK(end, "field latest access not in DarkLiveRangeMap");
                auto seqs = lrs.findLiveRangeSplits(*begin, *end, s);
                LOG_DEBUG1("\nThere are " << seqs.size() << " possible live range splits from "
                                          << *begin << " to " << *end << " for " << s << ":");
                PHV::LiveRangeSplit::report(seqs);
            }
        });
    }

    Logging::FileLog::close(lrsLogfile);

    auto summaryFilename = Logging::PassManager::getNewLogFileName("phv_allocation_summary_"_cs);
    auto summaryLogfile = new Logging::FileLog(pipeId, summaryFilename, Logging::Mode::AUTO);

    if (!unallocated.empty()) {
        AllocatePHV::diagnoseFailures(unallocated, utils);
        AllocatePHV::formatAndThrowError(*alloc, unallocated);
    }

    Logging::FileLog::close(summaryLogfile);
    return root;
}

}  // namespace PHV
