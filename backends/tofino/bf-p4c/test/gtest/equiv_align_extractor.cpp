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

#include <iostream>
#include <sstream>
#include <list>

#include "gtest/gtest.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf-p4c/test/utils/super_cluster_builder.h"
#include "bf-p4c/logging/constrained_fields.h"
#include "bf-p4c/logging/group_constraint_extractor.h"

namespace P4::Test {

class EquivalentAlignExtractorTest : public TofinoBackendTest {
 protected:
    std::istringstream CLUSTER_WITHOUT_EQUIV_ALIGN = std::istringstream(R"(SUPERCLUSTER Uid: 1358
    slice lists:        [ ]
    rotational clusters:
        [[egress::eg_md.flags.pfc_wd_drop<1> meta [0:0]]]
    )");

    std::istringstream CLUSTER_WITH_EQUIV_ALIGN = std::istringstream(R"(SUPERCLUSTER Uid: 1957
    slice lists:
        [ egress::hdr.ipv4.total_len<16> ^0 ^bit[0..31] deparsed solitary no_split exact_containers mocha [0:15] ]
        [ egress::hdr.ipv6.payload_len<16> ^0 ^bit[0..47] deparsed solitary no_split exact_containers mocha [0:15] ]
    rotational clusters:
        [[egress::eg_md.checks.mtu<16> meta solitary no_split [0:15], egress::hdr.ipv4.total_len<16> ^0 ^bit[0..31] deparsed solitary no_split exact_containers mocha [0:15], egress::hdr.ipv6.payload_len<16> ^0 ^bit[0..47] deparsed solitary no_split exact_containers mocha [0:15]]]
    )");

    SuperClusterBuilder scb;
    std::list<PHV::SuperCluster*> groups;
    PhvInfo info;

    ConstrainedFieldMap fieldMap;
};

TEST_F(EquivalentAlignExtractorTest, IgnoresAlignedClusterWithSingleItem) {
    // Build supercluster
    std::optional<PHV::SuperCluster*> sc1 = scb.build_super_cluster(CLUSTER_WITHOUT_EQUIV_ALIGN);
    if (!sc1) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc1);

    // Add required fields
    info.add("egress::eg_md.flags.pfc_wd_drop"_cs, EGRESS, 1, 0, true, false);

    // Extract groups
    fieldMap = ConstrainedFieldMapBuilder::buildMap(info, groups);
    EquivalentAlignExtractor extractor(groups, fieldMap);

    // Assertions
    EXPECT_FALSE(extractor.isFieldInAnyGroup("egress::eg_md.flags.pfc_wd_drop"_cs));
}

TEST_F(EquivalentAlignExtractorTest, ExtractsAlignedClusterWithMoreItems) {
    // Build supercluster
    std::optional<PHV::SuperCluster*> sc1 = scb.build_super_cluster(CLUSTER_WITH_EQUIV_ALIGN);
    if (!sc1) FAIL() << "Failed to build the cluster!";
    groups.push_back(*sc1);

    // Add required fields
    info.add("egress::eg_md.checks.mtu"_cs, EGRESS, 16, 0, false, false);
    info.add("egress::hdr.ipv4.total_len"_cs, EGRESS, 16, 0, false, false);
    info.add("egress::hdr.ipv6.payload_len"_cs, EGRESS, 16, 0, false, false);

    // Extract groups
    fieldMap = ConstrainedFieldMapBuilder::buildMap(info, groups);
    EquivalentAlignExtractor extractor(groups, fieldMap);

    // Assertions
    for (auto &field : info) {
        EXPECT_TRUE(extractor.isFieldInAnyGroup(field.name));

        auto agroups = extractor.getGroups(field.name);
        ASSERT_EQ(agroups.size(), 1u) << "Failed with field: " << field;
        ASSERT_EQ(agroups[0]->size(), 3u) << "Failed with field: " << field;

        auto &group = *agroups[0];
        EXPECT_EQ(group[0].getParent().getName(), "egress::eg_md.checks.mtu"_cs);
        EXPECT_EQ(group[1].getParent().getName(), "egress::hdr.ipv4.total_len"_cs);
        EXPECT_EQ(group[2].getParent().getName(), "egress::hdr.ipv6.payload_len"_cs);
    }
}

}  // namespace P4::Test
