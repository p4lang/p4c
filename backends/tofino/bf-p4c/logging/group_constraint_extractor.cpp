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

#include "group_constraint_extractor.h"

using Group = GroupConstraintExtractor::Group;

bool GroupConstraintExtractor::isFieldInAnyGroup(const cstring &name) const {
    return fieldToGroupMap.find(name) != fieldToGroupMap.end();
}

void GroupConstraintExtractor::processSlice(unsigned groupId, const PHV::FieldSlice &slice,
                        const ConstrainedFieldMap &map) {
    auto &field = map.at(slice.field()->name);

    for (auto &ls : field.getSlices()) {
        if (slice.range() != ls.getRange()) continue;

        groups[groupId].push_back(ls);
        fieldToGroupMap[field.getName()].insert(groupId);
    }
}

std::vector<const Group*> GroupConstraintExtractor::getGroups(const cstring &name) const {
    BUG_CHECK(isFieldInAnyGroup(name), "Field %s is not present in any group!", name.c_str());

    std::vector<const Group*> result;
    for (auto &item : fieldToGroupMap.at(name)) {
        result.push_back(&groups[item]);
    }

    return result;
}

/* MauGroupExtractor */
bool MauGroupExtractor::superClusterContainsOnlySingleField(const PHV::SuperCluster *sc) const {
    cstring fieldName = ""_cs;
    return sc->all_of_fieldslices([&fieldName] (const PHV::FieldSlice &slice) {
        if (fieldName == "") fieldName = slice.field()->name;
        return fieldName == slice.field()->name;
    });
}

MauGroupExtractor::MauGroupExtractor(const std::list<PHV::SuperCluster*> &clusterGroups,
                                      const ConstrainedFieldMap &map) {
    for (auto &sc : clusterGroups) {
        if (superClusterContainsOnlySingleField(sc)) continue;

        groups.push_back(Group());
        unsigned groupId = groups.size() - 1;

        sc->forall_fieldslices([&] (const PHV::FieldSlice &slice) {
            processSlice(groupId, slice, map);
        });
    }
}

/* EquivalentAlignExtractor */
void EquivalentAlignExtractor::processCluster(const PHV::AlignedCluster *cluster,
    const ConstrainedFieldMap &map) {
    if (cluster->slices().size() < 2) return;

    groups.push_back(Group());
    unsigned groupId = groups.size() - 1;

    for (auto &slice : cluster->slices()) {
        processSlice(groupId, slice, map);
    }
}

EquivalentAlignExtractor::EquivalentAlignExtractor(
    const std::list<PHV::SuperCluster*> &superclusters, const ConstrainedFieldMap &map) {
    for (auto &sc : superclusters) {
        for (auto &rc : sc->clusters()) {
            for (auto &ac : rc->clusters()) {
                processCluster(ac, map);
            }
        }
    }
}
