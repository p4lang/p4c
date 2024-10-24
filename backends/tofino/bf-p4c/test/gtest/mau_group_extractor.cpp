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

#include <iostream>
#include <list>
#include <sstream>

#include "bf-p4c/logging/constrained_fields.h"
#include "bf-p4c/logging/group_constraint_extractor.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf-p4c/test/utils/super_cluster_builder.h"
#include "gtest/gtest.h"

namespace P4::Test {

class MauGroupExtractorTest : public TofinoBackendTest {
 protected:
    std::istringstream CLUSTER_WITH_SINGLE_SLICE = std::istringstream(R"(SUPERCLUSTER Uid: 1957
    slice lists:  [ ]
    rotational clusters:
        [[egress::eg_intr_md.$valid<1> pov [0:0]]]
)");

    std::istringstream CLUSTER_WITH_WHOLE_SLICES = std::istringstream(R"(SUPERCLUSTER Uid: 1952
    slice lists:
        [ ingress::hdr.arp.$valid<1> pov [0:0]
        ingress::hdr.cpu.$valid<1> pov [0:0]
        ingress::hdr.vn_tag.$valid<1> pov [0:0] ]
    rotational clusters:
        [[ingress::hdr.arp.$valid<1> pov [0:0]]]
        [[ingress::hdr.cpu.$valid<1> pov [0:0]]]
        [[ingress::hdr.vn_tag.$valid<1> pov [0:0]]]
)");

    std::istringstream CLUSTER_WITH_PARTIAL_SLICES = std::istringstream(R"(SUPERCLUSTER Uid: 1954
    slice lists:
        [ ingress::hdr.vlan_tag.$stkvalid<2> meta pov no_split [0:0]
        ingress::hdr.vlan_tag.$stkvalid<2> meta pov no_split [1:1]
        ingress::dummy.dummy<2> [0:1] ]
    rotational clusters:
        [[ingress::hdr.vlan_tag.$stkvalid<2> meta pov no_split [0:0]]]
        [[ingress::hdr.vlan_tag.$stkvalid<2> meta pov no_split [1:1]]]
        [[ingress::dummy.dummy<2> [0:1]]]
)");

    std::istringstream CLUSTER_MULTI_A = std::istringstream(R"(SUPERCLUSTER Uid: 1546
    slice lists:        [ ]
    rotational clusters:
        [[ingress::Millstone.Guion.Wilmore<3> meta [0:0]], [ingress::Millstone.LaMoille.Philbrook<3> meta [0:0]], [ingress::Millstone.Hapeville.Luzerne<1> meta [0:0]], [ingress::Millstone.LaMoille.Skyway<3> meta [0:0]]]
)");

    std::istringstream CLUSTER_MULTI_B = std::istringstream(R"(SUPERCLUSTER Uid: 1547
    slice lists:        [ ]
    rotational clusters:
        [[ingress::Millstone.Guion.Wilmore<3> meta [1:2]], [ingress::Millstone.LaMoille.Philbrook<3> meta [1:2]], [ingress::Millstone.LaMoille.Skyway<3> meta [1:2]]]
)");

    SuperClusterBuilder scb;
    std::list<PHV::SuperCluster *> groups;
    PhvInfo info;

    ConstrainedFieldMap fieldMap;
};

TEST_F(MauGroupExtractorTest, GetGroupThrowsOnEmpty) {
    MauGroupExtractor extractor(groups, fieldMap);

    EXPECT_THROW({ extractor.getGroups("dummy"_cs); }, Util::CompilerBug);
}

TEST_F(MauGroupExtractorTest, IgnoresSuperClustersWithSingleField) {
    // Build all superclusters
    std::optional<PHV::SuperCluster *> sc1 = scb.build_super_cluster(CLUSTER_WITH_SINGLE_SLICE);
    if (!sc1) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc1);

    // Add required fields
    info.add("egress::eg_intr_md.$valid"_cs, EGRESS, 1, 0, false, true);

    // Extract groups
    fieldMap = ConstrainedFieldMapBuilder::buildMap(info, groups);
    MauGroupExtractor extractor(groups, fieldMap);

    // Assertions
    EXPECT_FALSE(extractor.isFieldInAnyGroup("egress::eg_intr_md.$valid"_cs));
}

TEST_F(MauGroupExtractorTest, ExtractsWholeSlices) {
    // Build all superclusters
    std::optional<PHV::SuperCluster *> sc1 = scb.build_super_cluster(CLUSTER_WITH_WHOLE_SLICES);
    if (!sc1) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc1);

    // Add required fields
    info.add("ingress::hdr.arp.$valid"_cs, INGRESS, 1, 0, false, true);
    info.add("ingress::hdr.cpu.$valid"_cs, INGRESS, 1, 0, false, true);
    info.add("ingress::hdr.vn_tag.$valid"_cs, INGRESS, 1, 0, false, true);

    // Extract groups
    fieldMap = ConstrainedFieldMapBuilder::buildMap(info, groups);
    MauGroupExtractor extractor(groups, fieldMap);

    // Assertions
    for (auto &field : info) {
        EXPECT_TRUE(extractor.isFieldInAnyGroup(field.name));

        auto mgroups = extractor.getGroups(field.name);
        ASSERT_EQ(mgroups.size(), 1u) << "Failed with field: " << field;
        ASSERT_EQ(mgroups[0]->size(), 3u) << "Failed with field: " << field;
    }

    auto mgroups = extractor.getGroups("ingress::hdr.vn_tag.$valid"_cs);
    auto &group = *mgroups[0];

    EXPECT_EQ(group[0].getParent().getName(), "ingress::hdr.arp.$valid"_cs);
    EXPECT_EQ(group[0].getRange(), le_bitrange(0, 0));
    EXPECT_EQ(group[1].getParent().getName(), "ingress::hdr.cpu.$valid"_cs);
    EXPECT_EQ(group[1].getRange(), le_bitrange(0, 0));
    EXPECT_EQ(group[2].getParent().getName(), "ingress::hdr.vn_tag.$valid"_cs);
    EXPECT_EQ(group[2].getRange(), le_bitrange(0, 0));
}

TEST_F(MauGroupExtractorTest, ExtractsPartialSlices) {
    // Build all superclusters
    std::optional<PHV::SuperCluster *> sc1 = scb.build_super_cluster(CLUSTER_WITH_PARTIAL_SLICES);
    if (!sc1) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc1);

    // Add required fields
    info.add("ingress::hdr.vlan_tag.$stkvalid"_cs, INGRESS, 2, 0, true, true);
    info.add("ingress::dummy.dummy"_cs, INGRESS, 2, 0, false, false);

    // Extract groups
    fieldMap = ConstrainedFieldMapBuilder::buildMap(info, groups);
    MauGroupExtractor extractor(groups, fieldMap);

    // Assertions
    EXPECT_TRUE(extractor.isFieldInAnyGroup("ingress::hdr.vlan_tag.$stkvalid"_cs));

    auto mgroups = extractor.getGroups("ingress::hdr.vlan_tag.$stkvalid"_cs);
    ASSERT_EQ(mgroups.size(), 1u);

    auto &group = *mgroups[0];
    ASSERT_EQ(group.size(), 3u);  // three items in group (2 slices + dummy)

    EXPECT_EQ(group[0].getParent().getName(), "ingress::hdr.vlan_tag.$stkvalid"_cs);
    EXPECT_EQ(group[0].getRange(), le_bitrange(0, 0));
    EXPECT_EQ(group[1].getParent().getName(), "ingress::hdr.vlan_tag.$stkvalid"_cs);
    EXPECT_EQ(group[1].getRange(), le_bitrange(1, 1));
    EXPECT_EQ(group[2].getParent().getName(), "ingress::dummy.dummy"_cs);
    EXPECT_EQ(group[2].getRange(), le_bitrange(0, 1));
}

TEST_F(MauGroupExtractorTest, ExtractFieldInMultipleGroups) {
    // Build all superclusters
    std::optional<PHV::SuperCluster *> sc1 = scb.build_super_cluster(CLUSTER_MULTI_A);
    if (!sc1) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc1);

    std::optional<PHV::SuperCluster *> sc2 = scb.build_super_cluster(CLUSTER_MULTI_B);
    if (!sc2) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc2);

    // Add required fields
    info.add("ingress::Millstone.Guion.Wilmore"_cs, INGRESS, 3, 0, true, false);
    info.add("ingress::Millstone.LaMoille.Philbrook"_cs, INGRESS, 3, 0, true, false);
    info.add("ingress::Millstone.Hapeville.Luzerne"_cs, INGRESS, 3, 0, true, false);
    info.add("ingress::Millstone.LaMoille.Skyway"_cs, INGRESS, 3, 0, true, false);

    // Extract groups
    fieldMap = ConstrainedFieldMapBuilder::buildMap(info, groups);
    MauGroupExtractor extractor(groups, fieldMap);

    // Assertions
    EXPECT_TRUE(extractor.isFieldInAnyGroup("ingress::Millstone.Guion.Wilmore"_cs));

    auto mgroups = extractor.getGroups("ingress::Millstone.Guion.Wilmore"_cs);
    ASSERT_EQ(mgroups.size(), 2u);

    ASSERT_EQ(mgroups[0]->size(), 4u);  // cluster_multi_a
    ASSERT_EQ(mgroups[1]->size(), 3u);  // cluster_multi_b

    auto &group0 = *mgroups[0];
    auto &group1 = *mgroups[1];

    EXPECT_EQ(group0[0].getParent().getName(), "ingress::Millstone.Guion.Wilmore"_cs);
    EXPECT_EQ(group0[0].getRange(), le_bitrange(0, 0));
    EXPECT_EQ(group0[1].getParent().getName(), "ingress::Millstone.LaMoille.Philbrook"_cs);
    EXPECT_EQ(group0[1].getRange(), le_bitrange(0, 0));
    EXPECT_EQ(group0[2].getParent().getName(), "ingress::Millstone.Hapeville.Luzerne"_cs);
    EXPECT_EQ(group0[2].getRange(), le_bitrange(0, 0));
    EXPECT_EQ(group0[3].getParent().getName(), "ingress::Millstone.LaMoille.Skyway"_cs);
    EXPECT_EQ(group0[3].getRange(), le_bitrange(0, 0));

    EXPECT_EQ(group1[0].getParent().getName(), "ingress::Millstone.Guion.Wilmore"_cs);
    EXPECT_EQ(group1[0].getRange(), le_bitrange(1, 2));
    EXPECT_EQ(group1[1].getParent().getName(), "ingress::Millstone.LaMoille.Philbrook"_cs);
    EXPECT_EQ(group1[1].getRange(), le_bitrange(1, 2));
    EXPECT_EQ(group1[2].getParent().getName(), "ingress::Millstone.LaMoille.Skyway"_cs);
    EXPECT_EQ(group1[2].getRange(), le_bitrange(1, 2));
}

}  // namespace P4::Test
