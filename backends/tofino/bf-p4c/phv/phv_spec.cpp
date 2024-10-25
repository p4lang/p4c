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

#include "bf-p4c/phv/phv_spec.h"

#include <optional>
#include <sstream>

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "frontends/parsers/p4/p4parser.hpp"
#include "lib/bitvec.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

void PhvSpec::addType(PHV::Type t) {
    const size_t typeId = definedTypes.size();
    definedTypes.push_back(t);
    definedSizes.insert(t.size());
    definedKinds.insert(t.kind());
    typeIdMap[t] = typeId;
}

const std::vector<PHV::Type> &PhvSpec::containerTypes() const { return definedTypes; }

const std::set<PHV::Size> &PhvSpec::containerSizes() const { return definedSizes; }

const std::set<PHV::Kind> &PhvSpec::containerKinds() const { return definedKinds; }

bool PhvSpec::hasContainerKind(PHV::Kind kind) const { return definedKinds.count(kind); }

const std::map<PHV::Size, std::set<PHV::Type>> PhvSpec::groupsToTypes() const {
    return sizeToTypeMap;
}

unsigned PhvSpec::numContainerTypes() const { return definedTypes.size(); }

PHV::Type PhvSpec::idToContainerType(unsigned id) const {
    BUG_CHECK(id < definedTypes.size(), "Container type id %1% is out of range");
    return definedTypes[id];
}

unsigned PhvSpec::containerTypeToId(PHV::Type type) const { return typeIdMap.at(type); }

PHV::Container PhvSpec::idToContainer(unsigned id) const {
    // Ids are assigned to containers in ascending order by index, with the
    // different container types interleaved. For example, on Tofino the
    // sequence is as follows:
    //   B0, H0, W0, TB0, TH0, TW0, B1, H1, W1, ...
    const auto typeId = id % numContainerTypes();
    const auto index = id / numContainerTypes();
    return PHV::Container(idToContainerType(typeId), index);
}

unsigned PhvSpec::containerToId(PHV::Container container) const {
    // See idToContainer() for the details of id assignment.
    return container.index() * numContainerTypes() + containerTypeToId(container.type());
}

bitvec PhvSpec::filterContainerSet(const bitvec &set, PHV::Kind kind) const {
    bitvec result;
    for (auto member : set) {
        if (idToContainer(member).type().kind() == kind) result.setbit(member);
    }

    return result;
}

bitvec PhvSpec::filterContainerSet(const bitvec &set, PHV::Type type) const {
    bitvec result;
    for (auto member : set) {
        if (idToContainer(member).type() == type) result.setbit(member);
    }
    return result;
}

struct TypeComparator {
    bool operator()(const PHV::Type &left, const PHV::Type &right) const {
        if (left.kind() != right.kind()) return left.kind() > right.kind();
        return left.size() < right.size();
    }
};

cstring PhvSpec::containerSetToString(const bitvec &set) const {
    std::map<PHV::Type, std::set<unsigned>, TypeComparator> containersByType;
    for (auto member : set) {
        auto container = idToContainer(member);
        containersByType[container.type()].insert(container.index());
    }

    bool first = true;
    std::stringstream result;

    auto outputRange = [&](PHV::Type type, unsigned min, unsigned max) {
        if (!first) result << ", ";
        first = false;

        result << PHV::Container(type, min);
        if (min != max) result << "-" << max;
    };

    for (auto &kv : containersByType) {
        std::optional<unsigned> curRangeStart = std::nullopt;
        std::optional<unsigned> curRangeEnd = std::nullopt;
        for (auto idx : kv.second) {
            if (!curRangeStart) {
                curRangeStart = idx;
                curRangeEnd = idx;
                continue;
            }

            if (*curRangeEnd == idx - 1) {
                curRangeEnd = idx;
                continue;
            }

            outputRange(kv.first, *curRangeStart, *curRangeEnd);
            curRangeStart = idx;
            curRangeEnd = idx;
        }

        outputRange(kv.first, *curRangeStart, *curRangeEnd);
    }

    return cstring(result);
}

bitvec PhvSpec::range(PHV::Type t, unsigned start, unsigned length) const {
    bitvec containers;
    for (unsigned index = start; index < start + length; ++index)
        containers.setbit(index * numContainerTypes() + containerTypeToId(t));
    return containers;
}

const bitvec &PhvSpec::physicalContainers() const {
    if (physical_containers_i) return physical_containers_i;
    bitvec containers;
    for (auto sg : mauGroups())
        for (auto g : sg.second) containers |= g;
    for (auto g : tagalongCollections()) containers |= g;
    physical_containers_i = std::move(containers);
    return physical_containers_i;
}

bitvec PhvSpec::mauGroup(unsigned container_id) const {
    const auto containerType = idToContainerType(container_id % numContainerTypes());
    auto it = mauGroupSpec.find(containerType.size());
    if (it == mauGroupSpec.end()) return bitvec();

    // Check if container type is present in the mauGroupSpec.
    MauGroupType typeDesc = it->second;
    auto itDesc = typeDesc.types.find(containerType);
    if (itDesc == typeDesc.types.end()) return bitvec();

    // At this point, we have determined that the container type is present in the mauGroupSpec.
    const unsigned index = container_id / numContainerTypes();
    unsigned groupSize = itDesc->second;
    const unsigned mau_group_index = index / groupSize;
    auto group = mauGroups(containerType.size());
    if (mau_group_index < group.size()) return group[mau_group_index];
    return bitvec();
}

const std::map<PHV::Size, std::vector<bitvec>> &PhvSpec::mauGroups() const {
    if (!mau_groups_i.empty()) return mau_groups_i;

    std::map<PHV::Size, std::vector<bitvec>> mau_groups;
    for (auto sizeSpec : mauGroupSpec) {
        // numGroups: Number of groups of each container size.
        unsigned numGroups = sizeSpec.second.numGroups;
        // groupSize: Size of the group.
        PHV::Size groupSize = sizeSpec.first;
        // Generate one bitvec for every group in this size. The bitvec for each group is the union
        // of all bitvecs corresponding to each type of container within the group.
        for (unsigned index = 0; index < numGroups; index++) {
            // sizeRange: Representation of all containers in group index of size groupSize bits.
            bitvec sizeRange;
            for (auto typeSpec : sizeSpec.second.types) {
                // groupNumContainers: Number of containers in each type in group.
                unsigned groupNumContainers = typeSpec.second;
                // Type of container.
                PHV::Type t = typeSpec.first;
                // typeRange: Representation of all groupNumContainers of type t in group index.
                bitvec typeRange = range(t, index * groupNumContainers, groupNumContainers);
                LOG4("  Adding ID " << typeRange << " for type " << t);
                sizeRange |= typeRange;
            }
            LOG4("Adding ID " << sizeRange << " corresponding to size " << groupSize);
            mau_groups[groupSize].push_back(sizeRange);
        }
    }
    mau_groups_i = std::move(mau_groups);
    return mau_groups_i;
}

bitvec PhvSpec::ingressOrEgressOnlyContainers(
    const std::map<PHV::Size, std::vector<unsigned>> &gressOnlyMauGroupIds) const {
    bitvec containers;
    for (auto gg : gressOnlyMauGroupIds) {
        PHV::Size size = gg.first;
        auto groupIDs = gg.second;
        MauGroupType sizeSpec = mauGroupSpec.at(size);
        for (auto typeSpec : sizeSpec.types) {
            unsigned groupSize = typeSpec.second;
            PHV::Type type = typeSpec.first;
            for (auto groupID : groupIDs) containers |= range(type, groupID * groupSize, groupSize);
        }
    }
    return containers;
}

const bitvec &PhvSpec::ingressOnly() const {
    if (ingress_only_containers_i) return ingress_only_containers_i;
    ingress_only_containers_i = ingressOrEgressOnlyContainers(ingressOnlyMauGroupIds);
    return ingress_only_containers_i;
}

const bitvec &PhvSpec::egressOnly() const {
    if (egress_only_containers_i) return egress_only_containers_i;
    egress_only_containers_i = ingressOrEgressOnlyContainers(egressOnlyMauGroupIds);
    return egress_only_containers_i;
}

unsigned PhvSpec::getTagalongCollectionId(PHV::Container c) const {
    BUG_CHECK(c.is(PHV::Kind::tagalong), "container is not tagalong");

    auto tagalongCollectionSpec = getTagalongCollectionSpec();
    auto type_in_collection = tagalongCollectionSpec.at(c.type());

    return c.index() / type_in_collection;
}

const std::vector<bitvec> &PhvSpec::tagalongCollections() const {
    if (!tagalong_collections_i.empty()) return tagalong_collections_i;
    std::vector<bitvec> tagalong_collections;

    for (unsigned coll_num = 0; coll_num < numTagalongCollections; ++coll_num) {
        bitvec collection;
        for (auto cs : tagalongCollectionSpec) {
            PHV::Type coll_type = cs.first;
            unsigned coll_size = cs.second;
            collection |= range(coll_type, coll_num * coll_size, coll_size);
        }
        tagalong_collections.push_back(collection);
    }

    tagalong_collections_i = std::move(tagalong_collections);
    return tagalong_collections_i;
}

bitvec PhvSpec::tagalongCollection(unsigned container_id) const {
    const auto containerType = idToContainerType(container_id % numContainerTypes());
    if (containerType.kind() != PHV::Kind::tagalong) return bitvec();

    const unsigned index = container_id / numContainerTypes();
    unsigned collection_num = index / tagalongCollectionSpec.at(containerType);

    if (tagalongCollections().size() <= collection_num) return bitvec();

    return tagalongCollections()[collection_num];
}

const std::vector<bitvec> &PhvSpec::mauGroups(PHV::Size sz) const {
    auto it = mauGroups().find(sz);
    if (it != mauGroups().end()) return mauGroups().at(sz);
    static std::vector<bitvec> dummy;
    return dummy;
}

const std::pair<int, int> PhvSpec::mauGroupNumAndSize(const PHV::Type t) const {
    if (!mauGroupSpec.count(t.size())) return std::pair<int, int>();
    // Find the size specification corresponding to the size of the container.
    MauGroupType sizeSpec = mauGroupSpec.at(t.size());
    unsigned numGroups = sizeSpec.numGroups;
    // If that sized container does not have the particular type of container, return an empty pair.
    if (!sizeSpec.types.count(t)) return std::pair<int, int>();
    unsigned groupSize = sizeSpec.types.at(t);
    return std::pair<int, int>(numGroups, groupSize);
}

const std::pair<int, int> PhvSpec::deparserGroupNumAndSize(const PHV::Type t) const {
    if (!deparserGroupSpec.count(t)) return std::pair<int, int>();
    return deparserGroupSpec.at(t);
}

bitvec PhvSpec::deparserGroup(unsigned id) const {
    const auto containerType = idToContainerType(id % numContainerTypes());
    const unsigned index = id / numContainerTypes();

    // Individually assigned containers aren't part of a group, by definition.
    if (individuallyAssignedContainers()[id]) return range(containerType, index, 1);

    // We also treat overflow containers (i.e., containers which don't exist in
    // hardware) as being individually assigned.
    if (!physicalContainers()[id]) return range(containerType, index, 1);

    if (containerType.kind() == PHV::Kind::tagalong) return tagalongCollection(id);

    // Outside of the exceptional cases above, containers are assigned to
    // threads in groups. The grouping depends on the type of container.

    auto find = deparserGroupSize.find(containerType);

    if (find == deparserGroupSize.end()) return bitvec();

    unsigned groupSize = deparserGroupSize.at(containerType);
    return range(containerType, (index / groupSize) * groupSize, groupSize) & physicalContainers();
}

unsigned PhvSpec::getContainersPerGroup(
    const std::map<PHV::Size, unsigned> &numContainersPerGroup) const {
    unsigned rv = 0;
    // Check that the number of containers in each MAU group is the same (applicable only to Tofino
    // and Tofino2).
    for (auto kv : numContainersPerGroup) {
        if (rv == 0) {
            rv = kv.second;
            continue;
        }
        BUG_CHECK(rv == kv.second,
                  "Neither Tofino nor Tofino2 support MAU groups of different sizes");
    }
    return rv;
}

unsigned PhvSpec::physicalAddress(const PHV::Container &c, ArchBlockType_t interface) const {
    return physicalAddress(containerToId(c), interface);
}

std::optional<PHV::Container> PhvSpec::physicalAddressToContainer(unsigned address,
                                                                  ArchBlockType_t interface) const {
    for (auto &[type, physicalRange] : physicalAddressSpec(interface)) {
        for (unsigned i = 0; i < physicalRange.blocks; i++) {
            unsigned lo = physicalRange.start + i * physicalRange.incr;
            unsigned hi =
                lo + ((physicalRange.blockSize << physicalRange.shl) >> physicalRange.shr);
            if (address >= lo && address < hi) {
                unsigned base = i * physicalRange.blockSize;
                unsigned offset = ((address - lo) << physicalRange.shr) >> physicalRange.shl;
                unsigned index = base + offset;
                return PHV::Container(type, index);
            }
        }
    }
    return std::nullopt;
}

PhvSpec::AddressSpec TofinoPhvSpec::_physicalAddresses = {
    /*                  start, blocks, blockSize, incr, shl, shr */
    {PHV::Type::W, {0, 1, 64, 0, 0, 0}},    {PHV::Type::B, {64, 1, 64, 0, 0, 0}},
    {PHV::Type::H, {128, 1, 128, 0, 0, 0}}, {PHV::Type::TW, {256, 1, 32, 0, 0, 0}},
    {PHV::Type::TB, {288, 1, 32, 0, 0, 0}}, {PHV::Type::TH, {320, 1, 48, 0, 0, 0}}};

TofinoPhvSpec::TofinoPhvSpec() {
    addType(PHV::Type::B);
    addType(PHV::Type::H);
    addType(PHV::Type::W);
    addType(PHV::Type::TB);
    addType(PHV::Type::TH);
    addType(PHV::Type::TW);

    auto phv_scale_factor = BackendOptions().phv_scale_factor;

    BUG_CHECK(unsigned(phv_scale_factor * 4) >= 1, "PHV scale factor %1% too small",
              phv_scale_factor);

    std::map<PHV::Type, std::pair<unsigned, unsigned>> rawMauGroupSpec = {
        {PHV::Type::B, std::make_pair(4 * phv_scale_factor, 16)},
        {PHV::Type::H, std::make_pair(6 * phv_scale_factor, 16)},
        {PHV::Type::W, std::make_pair(4 * phv_scale_factor, 16)}};

    std::map<PHV::Size, unsigned> numContainersPerGroup;
    for (auto kv : rawMauGroupSpec) {
        PHV::Type t = kv.first;
        unsigned numGroups = kv.second.first;
        unsigned numContainers = kv.second.second;
        if (numContainersPerGroup.count(t.size()))
            BUG_CHECK(numContainersPerGroup[t.size()] == numContainers,
                      "Neither Tofino nor Tofino2 support MAU groups of different sizes.");
        else
            numContainersPerGroup[t.size()] = numContainers;
        sizeToTypeMap[t.size()].insert(t);
        MauGroupType desc(numGroups);
        desc.addType(t, numContainers);
        mauGroupSpec[t.size()] = desc;
    }
    containersPerGroup = getContainersPerGroup(numContainersPerGroup);

    if (LOGGING(4)) {
        for (auto kv : mauGroupSpec) {
            LOG4("    " << kv.second.numGroups << " groups of size " << kv.first);
            for (auto kv1 : kv.second.types) LOG4("      " << kv1.first << " : " << kv1.second);
        }
    }

    std::vector<unsigned> ingressGroupIds;
    for (unsigned i = 0; i < (unsigned)phv_scale_factor; ++i) {
        LOG4("    Setting group " << (4 * i) << " to ingress only");
        ingressGroupIds.push_back(4 * i);
    }

    ingressOnlyMauGroupIds = {{PHV::Size::b8, ingressGroupIds},
                              {PHV::Size::b16, ingressGroupIds},
                              {PHV::Size::b32, ingressGroupIds}};

    std::vector<unsigned> egressGroupIds;
    for (unsigned i = 0; i < (unsigned)phv_scale_factor; ++i) {
        LOG4("    Setting group " << (4 * i + 1) << " to egress only");
        egressGroupIds.push_back(4 * i + 1);
    }

    egressOnlyMauGroupIds = {{PHV::Size::b8, egressGroupIds},
                             {PHV::Size::b16, egressGroupIds},
                             {PHV::Size::b32, egressGroupIds}};

    tagalongCollectionSpec = {{PHV::Type::TB, 4 * phv_scale_factor},
                              {PHV::Type::TH, 6 * phv_scale_factor},
                              {PHV::Type::TW, 4 * phv_scale_factor}};

    numTagalongCollections = 8 * phv_scale_factor;

    if (BackendOptions().no_tagalong) numTagalongCollections = 0;

    deparserGroupSize = {{PHV::Type::B, 8 * phv_scale_factor},
                         {PHV::Type::H, 8 * phv_scale_factor},
                         {PHV::Type::W, 4 * phv_scale_factor}};

    deparserGroupSpec = {{PHV::Type::B, std::make_pair(8, 8 * phv_scale_factor)},
                         {PHV::Type::H, std::make_pair(12, 8 * phv_scale_factor)},
                         {PHV::Type::W, std::make_pair(16, 4 * phv_scale_factor)}};

    numPovBits = 256;
}

bitvec TofinoPhvSpec::parserGroup(unsigned id) const { return bitvec(id, 1); }

bool TofinoPhvSpec::hasParserExtractGroups() const { return false; }

bitvec TofinoPhvSpec::parserExtractGroup(unsigned id) const { return bitvec(id, 1); }

unsigned TofinoPhvSpec::parserGroupId(const PHV::Container &c) const {
    // On Tofino, the mapping from parser to MAU is identical
    return mauGroupId(c);
}

unsigned TofinoPhvSpec::mauGroupId(const PHV::Container &c) const {
    // Get MAU group specific information from phvSpec -- this is silly!
    std::pair<int, int> numBytes = mauGroupNumAndSize(PHV::Type::B);
    std::pair<int, int> numHalfs = mauGroupNumAndSize(PHV::Type::H);
    std::pair<int, int> numWords = mauGroupNumAndSize(PHV::Type::W);

    int groupID = -1;
    if (std::optional<bitvec> mg = mauGroup(c.index())) {
        // groupID represents the unique string that identifies an MAU group
        if (c.type() == PHV::Type::B) {
            groupID = (c.index() / numBytes.second) + numWords.first;
        } else if (c.type() == PHV::Type::H) {
            groupID = (c.index() / numHalfs.second) + numWords.first + numBytes.first;
        } else if (c.type() == PHV::Type::W) {
            groupID = c.index() / numWords.second;
        }
    }
    return groupID;
}

unsigned TofinoPhvSpec::deparserGroupId(const PHV::Container &c) const {
    // On Tofino, the mapping from MAU to deparser is identical
    return mauGroupId(c);
}

const bitvec &TofinoPhvSpec::individuallyAssignedContainers() const {
    if (individually_assigned_containers_i) return individually_assigned_containers_i;
    bitvec containers =
        range(PHV::Type::B, 56, 8) | range(PHV::Type::H, 88, 8) | range(PHV::Type::W, 60, 4);
    individually_assigned_containers_i = std::move(containers);
    return individually_assigned_containers_i;
}

unsigned TofinoPhvSpec::physicalAddress(unsigned id, ArchBlockType_t /* interface */) const {
    const PHV::Type containerType = idToContainerType(id % numContainerTypes());
    const unsigned index = id / numContainerTypes();
    BUG_CHECK(_physicalAddresses.find(containerType) != _physicalAddresses.end(),
              "PHV container %1% has unrecognized type %2%", idToContainer(id), containerType);

    auto physicalRange = _physicalAddresses.at(containerType);
    auto rv = physicalRange.start + index;
    BUG_CHECK(index < physicalRange.blockSize, "No physical address for PHV container %1%",
              idToContainer(id));

    return rv;
}

// Static data member intializers for phv grouping on JBay
PhvSpec::AddressSpec JBayPhvSpec::_physicalMauAddresses = {
    /*                  start, blocks, blockSize, incr, shl, shr */
    {PHV::Type::W, {0, 4, 12, 20, 0, 0}},   {PHV::Type::MW, {12, 4, 4, 20, 0, 0}},
    {PHV::Type::DW, {16, 4, 4, 20, 0, 0}},  {PHV::Type::B, {80, 4, 12, 20, 0, 0}},
    {PHV::Type::MB, {92, 4, 4, 20, 0, 0}},  {PHV::Type::DB, {96, 4, 4, 20, 0, 0}},
    {PHV::Type::H, {160, 6, 12, 20, 0, 0}}, {PHV::Type::MH, {172, 6, 4, 20, 0, 0}},
    {PHV::Type::DH, {176, 6, 4, 20, 0, 0}}};
PhvSpec::AddressSpec JBayPhvSpec::_physicalParserAddresses = {
    /*                  start, blocks, blockSize, incr, shl, shr */
    {PHV::Type::W, {0, 4, 12, 32, 1, 0}},   {PHV::Type::MW, {24, 4, 4, 32, 1, 0}},
    {PHV::Type::B, {128, 4, 12, 8, 0, 1}},  {PHV::Type::MB, {134, 4, 4, 8, 0, 1}},
    {PHV::Type::H, {160, 6, 12, 16, 0, 0}}, {PHV::Type::MH, {172, 6, 4, 16, 0, 0}},
};
PhvSpec::AddressSpec JBayPhvSpec::_physicalDeparserAddresses = {
    /*                  start, blocks, blockSize, incr, shl, shr */
    {PHV::Type::W, {0, 4, 12, 16, 0, 0}},   {PHV::Type::MW, {12, 4, 4, 16, 0, 0}},
    {PHV::Type::B, {64, 4, 12, 16, 0, 0}},  {PHV::Type::MB, {76, 4, 4, 16, 0, 0}},
    {PHV::Type::H, {128, 6, 12, 16, 0, 0}}, {PHV::Type::MH, {140, 6, 4, 16, 0, 0}},
};

JBayPhvSpec::JBayPhvSpec() {
    addType(PHV::Type::W);
    addType(PHV::Type::MW);
    addType(PHV::Type::DW);
    addType(PHV::Type::B);
    addType(PHV::Type::MB);
    addType(PHV::Type::DB);
    addType(PHV::Type::H);
    addType(PHV::Type::MH);
    addType(PHV::Type::DH);

    auto phv_scale_factor = BackendOptions().phv_scale_factor;
    if (phv_scale_factor != 1.0)
        P4C_UNIMPLEMENTED("phv_scale_factor not yet implemented for Tofino2");

    std::map<PHV::Size, std::map<unsigned, std::map<PHV::Type, unsigned>>> rawMauGroupSpec = {
        {PHV::Size::b8, {{4, {{PHV::Type::B, 12}, {PHV::Type::MB, 4}, {PHV::Type::DB, 4}}}}},
        {PHV::Size::b16, {{6, {{PHV::Type::H, 12}, {PHV::Type::MH, 4}, {PHV::Type::DH, 4}}}}},
        {PHV::Size::b32, {{4, {{PHV::Type::W, 12}, {PHV::Type::MW, 4}, {PHV::Type::DW, 4}}}}}};

    std::map<PHV::Size, unsigned> numContainersPerGroup;
    for (auto kv : rawMauGroupSpec) {
        PHV::Size sz = kv.first;
        numContainersPerGroup[sz] = 0;
        for (auto kv1 : kv.second) {
            unsigned numGroups = kv1.first;
            std::map<PHV::Type, unsigned> types = kv1.second;
            for (auto kv2 : types) {
                PHV::Type t = kv2.first;
                sizeToTypeMap[sz].insert(t);
                numContainersPerGroup[sz] += kv2.second;
            }
            MauGroupType desc(numGroups, types);
            mauGroupSpec[sz] = desc;
        }
    }
    containersPerGroup = getContainersPerGroup(numContainersPerGroup);

    if (LOGGING(4)) {
        for (auto kv : mauGroupSpec) {
            LOG4("    " << kv.second.numGroups << " groups of size " << kv.first);
            for (auto kv1 : kv.second.types) LOG4("      " << kv1.first << " : " << kv1.second);
        }
    }

    ingressOnlyMauGroupIds = {};

    egressOnlyMauGroupIds = {};

    tagalongCollectionSpec = {};

    numTagalongCollections = 0;

    deparserGroupSize = {{PHV::Type::B, {4}},  {PHV::Type::MB, {4}}, {PHV::Type::H, {4}},
                         {PHV::Type::MH, {4}}, {PHV::Type::W, {2}},  {PHV::Type::MW, {2}}};

    deparserGroupSpec = {
        {PHV::Type::B, std::make_pair(16, 4)},  {PHV::Type::H, std::make_pair(24, 4)},
        {PHV::Type::W, std::make_pair(32, 2)},  {PHV::Type::MB, std::make_pair(16, 4)},
        {PHV::Type::MH, std::make_pair(24, 4)}, {PHV::Type::MW, std::make_pair(32, 2)}};

    numPovBits = 128;
}

bitvec JBayPhvSpec::parserGroup(unsigned id) const {
    if (idToContainer(id).is(PHV::Size::b8)) {
        // Return the even/odd pair.
        const auto containerType = idToContainerType(id % numContainerTypes());
        const unsigned index = id / numContainerTypes();
        bool isEven = index % 2 == 0;
        return range(containerType, isEven ? index : index - 1, 2) & physicalContainers();
    }
    return bitvec(id, 1);
}

bool JBayPhvSpec::hasParserExtractGroups() const { return false; }

bitvec JBayPhvSpec::parserExtractGroup(unsigned id) const { return bitvec(id, 1); }

unsigned JBayPhvSpec::parserGroupId(const PHV::Container &c) const {
    // Get de/parser group specific information from phvSpec -- this is silly!
    std::pair<int, int> numBytes = deparserGroupNumAndSize(PHV::Type::B);
    std::pair<int, int> numHalfs = deparserGroupNumAndSize(PHV::Type::H);
    std::pair<int, int> numWords = deparserGroupNumAndSize(PHV::Type::W);
    std::pair<int, int> numMochaBytes = deparserGroupNumAndSize(PHV::Type::MB);
    std::pair<int, int> numMochaHalfs = deparserGroupNumAndSize(PHV::Type::MH);
    std::pair<int, int> numMochaWords = deparserGroupNumAndSize(PHV::Type::MW);

    int groupID = -1;
    if (std::optional<bitvec> mg = parserGroup(c.index())) {
        // groupID represents the unique string that identifies an MAU group
        if (c.type() == PHV::Type::B) {
            groupID = (c.index() / numBytes.second) + numWords.first;
        } else if (c.type() == PHV::Type::H) {
            groupID = (c.index() / numHalfs.second) + numWords.first + numBytes.first;
        } else if (c.type() == PHV::Type::W) {
            groupID = c.index() / numWords.second;
        } else if (c.type() == PHV::Type::MB) {
            groupID = (c.index() / numMochaBytes.second) + numMochaWords.first;
        } else if (c.type() == PHV::Type::MH) {
            groupID =
                (c.index() / numMochaHalfs.second) + numMochaWords.first + numMochaBytes.first;
        } else if (c.type() == PHV::Type::MW) {
            groupID = c.index() / numMochaWords.second;
        }
    }
    return groupID;
}

unsigned JBayPhvSpec::mauGroupId(const PHV::Container &c) const {
    // Get MAU group specific information from phvSpec -- this is silly!
    std::pair<int, int> numBytes = mauGroupNumAndSize(PHV::Type::B);
    std::pair<int, int> numHalfs = mauGroupNumAndSize(PHV::Type::H);
    std::pair<int, int> numWords = mauGroupNumAndSize(PHV::Type::W);
    std::pair<int, int> numMochaBytes = mauGroupNumAndSize(PHV::Type::MB);
    std::pair<int, int> numMochaHalfs = mauGroupNumAndSize(PHV::Type::MH);
    std::pair<int, int> numMochaWords = mauGroupNumAndSize(PHV::Type::MW);
    std::pair<int, int> numDarkBytes = mauGroupNumAndSize(PHV::Type::DB);
    std::pair<int, int> numDarkHalfs = mauGroupNumAndSize(PHV::Type::DH);
    std::pair<int, int> numDarkWords = mauGroupNumAndSize(PHV::Type::DW);

    int groupID = -1;
    if (std::optional<bitvec> mg = mauGroup(c.index())) {
        // groupID represents the unique string that identifies an MAU group
        if (c.type() == PHV::Type::B) {
            groupID = (c.index() / numBytes.second) + numWords.first;
        } else if (c.type() == PHV::Type::H) {
            groupID = (c.index() / numHalfs.second) + numWords.first + numBytes.first;
        } else if (c.type() == PHV::Type::W) {
            groupID = c.index() / numWords.second;
        } else if (c.type() == PHV::Type::MB) {
            groupID = (c.index() / numMochaBytes.second) + numMochaWords.first;
        } else if (c.type() == PHV::Type::MH) {
            groupID =
                (c.index() / numMochaHalfs.second) + numMochaWords.first + numMochaBytes.first;
        } else if (c.type() == PHV::Type::MW) {
            groupID = c.index() / numMochaWords.second;
        } else if (c.type() == PHV::Type::DB) {
            groupID = (c.index() / numDarkBytes.second) + numDarkWords.first;
        } else if (c.type() == PHV::Type::DH) {
            groupID = (c.index() / numDarkHalfs.second) + numDarkWords.first + numDarkBytes.first;
        } else if (c.type() == PHV::Type::DW) {
            groupID = (c.index() / numDarkWords.second);
        }
    }
    return groupID;
}

unsigned JBayPhvSpec::deparserGroupId(const PHV::Container &c) const {
    // parser and deparser are the same on JBay.
    return parserGroupId(c);
}

const bitvec &JBayPhvSpec::individuallyAssignedContainers() const {
    return individually_assigned_containers_i;
}

unsigned JBayPhvSpec::physicalAddress(unsigned id, ArchBlockType_t interface) const {
    const PHV::Type containerType = idToContainerType(id % numContainerTypes());
    const unsigned index = id / numContainerTypes();
    RangeSpec physicalRange;
    if (interface == MAU) {
        BUG_CHECK(_physicalMauAddresses.find(containerType) != _physicalMauAddresses.end(),
                  "PHV container %1% has unrecognized type %2%", idToContainer(id), containerType);
        physicalRange = _physicalMauAddresses.at(containerType);
    } else if (interface == PARSER) {
        BUG_CHECK(_physicalParserAddresses.find(containerType) != _physicalParserAddresses.end(),
                  "PHV container %1% has unrecognized type %2%", idToContainer(id), containerType);
        physicalRange = _physicalParserAddresses.at(containerType);
    } else {
        BUG_CHECK(
            _physicalDeparserAddresses.find(containerType) != _physicalDeparserAddresses.end(),
            "PHV container %1% has unrecognized type %2%", idToContainer(id), containerType);
        physicalRange = _physicalDeparserAddresses.at(containerType);
    }

    auto block = index / physicalRange.blockSize;
    auto blockOffset = index % physicalRange.blockSize;

    BUG_CHECK(block < physicalRange.blocks, "No valid block for PHV container %1%",
              idToContainer(id));
    BUG_CHECK(blockOffset < physicalRange.blockSize, "No physical address for PHV container %1%",
              idToContainer(id));

    return physicalRange.start + block * physicalRange.incr +
           ((blockOffset << physicalRange.shl) >> physicalRange.shr);
}

void PhvSpec::applyGlobalPragmas(const std::vector<const IR::Annotation *> &global_pragmas) const {
    // clear all the cached values
    physical_containers_i.clear();
    mau_groups_i.clear();
    ingress_only_containers_i.clear();
    egress_only_containers_i.clear();
    tagalong_collections_i.clear();
    individually_assigned_containers_i.clear();
    std::set<PHV::Type> typesSeen;
    for (auto *annot : global_pragmas) {
        if (annot->name != PragmaPhvLimit::name) continue;
        physicalContainers();  // create the cache if needed
        PHV::Container startRange, prev;
        bool negate = false;
        for (auto *tok : annot->body) {
            PHV::Container c(tok->text, false);
            if (startRange) {
                if (tok->token_type == P4::P4Parser::token_type::TOK_INTEGER)
                    c = PHV::Container(startRange.type(), atoi(tok->text));
                if (!c || c.type() != startRange.type() || startRange.index() > c.index()) {
                    error(ErrorType::ERR_INVALID, "invalid container range %2%-%3% in %1%", annot,
                          startRange, tok->text);
                } else if (!typeIdMap.count(c.type())) {
                    // container type not present on this target -- ignore;
                } else {
                    bitvec r =
                        range(c.type(), startRange.index(), c.index() - startRange.index() + 1);
                    if (negate)
                        physical_containers_i -= r;
                    else
                        physical_containers_i |= r;
                }
                startRange = PHV::Container();
            } else if (tok->text == ",") {
                negate = false;
            } else if (tok->text == "-") {
                if (!(startRange = prev)) negate = true;
            } else if (c) {
                if (typeIdMap.count(c.type())) {
                    if (!typesSeen.count(c.type())) {
                        typesSeen.insert(c.type());
                        if (!negate) {
                            physical_containers_i -=
                                filterContainerSet(physical_containers_i, c.type());
                        }
                        physical_containers_i[containerToId(c)] = !negate;
                    }
                }
            } else if (negate && tok->text == "D") {
                physical_containers_i -= filterContainerSet(physical_containers_i, PHV::Kind::dark);
            } else if (negate && tok->text == "M") {
                physical_containers_i -=
                    filterContainerSet(physical_containers_i, PHV::Kind::mocha);
            } else if (negate && tok->text == "T") {
                physical_containers_i -=
                    filterContainerSet(physical_containers_i, PHV::Kind::tagalong);
            } else {
                error(ErrorType::ERR_INVALID, "invalid container %2% in %1%", annot, tok->text);
            }
            prev = c;
        }
    }
    if (physical_containers_i) {
        // propagate assignment restrictions to other sets;
        mauGroups();
        for (auto &v : Values(mau_groups_i))
            for (auto &bv : v) bv &= physical_containers_i;
        ingressOnly();
        ingress_only_containers_i &= physical_containers_i;
        egressOnly();
        egress_only_containers_i &= physical_containers_i;
        tagalongCollections();
        for (auto &bv : tagalong_collections_i) bv &= physical_containers_i;
        individuallyAssignedContainers();
        individually_assigned_containers_i &= physical_containers_i;
    }
}
