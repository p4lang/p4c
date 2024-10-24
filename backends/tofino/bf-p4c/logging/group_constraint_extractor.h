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

#ifndef BF_P4C_LOGGING_GROUP_CONSTRAINT_EXTRACTOR_H_
#define BF_P4C_LOGGING_GROUP_CONSTRAINT_EXTRACTOR_H_

#include <map>
#include <set>
#include <vector>

#include "bf-p4c/logging/constrained_fields.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/utils/utils.h"

class PhvInfo;

/**
 *  \brief Blueprint for group constraint extractor classes
 */
class GroupConstraintExtractor {
 public:
    using Group = std::vector<ConstrainedSlice>;

 protected:
    std::vector<Group> groups;
    std::map<cstring, std::set<unsigned>> fieldToGroupMap;  // Key is name of the field

    void processSlice(unsigned groupId, const PHV::FieldSlice &slice,
                      const ConstrainedFieldMap &map);

 public:
    std::vector<const Group *> getGroups(const cstring &name) const;

    // For a given field, check whether it is contained in any group
    bool isFieldInAnyGroup(const cstring &name) const;
};

/**
 *  Class for extracting MAU Group information from result of Clustering class
 */
class MauGroupExtractor : public GroupConstraintExtractor {
 public:
    using MauGroup = Group;

 private:
    bool superClusterContainsOnlySingleField(const PHV::SuperCluster *sc) const;

 public:
    MauGroupExtractor(const std::list<PHV::SuperCluster *> &clusterGroups,
                      const ConstrainedFieldMap &map);
};

/**
 *  Class for extracting Equivalent Alignment constraint from result of Clustering class
 */
class EquivalentAlignExtractor : public GroupConstraintExtractor {
 private:
    void processCluster(const PHV::AlignedCluster *cluster, const ConstrainedFieldMap &map);

 public:
    EquivalentAlignExtractor(const std::list<PHV::SuperCluster *> &superclusters,
                             const ConstrainedFieldMap &map);
};

#endif /* BF_P4C_LOGGING_GROUP_CONSTRAINT_EXTRACTOR_H_ */
