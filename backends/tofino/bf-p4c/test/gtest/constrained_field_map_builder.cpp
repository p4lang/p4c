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
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "bf-p4c/test/utils/super_cluster_builder.h"
#include "gtest/gtest.h"

namespace P4::Test {

class ConstrainedFieldMapBuilderTest : public TofinoBackendTest {
 protected:
    using AR = Constraints::AlignmentConstraint::AlignmentReason;

    std::istringstream SUPERCLUSTER = std::istringstream(R"(SUPERCLUSTER Uid: 41
    slice lists:
        [ ingress::hdr.test<32> ^0 [0:19]
          ingress::hdr.test<32> ^4 [20:31]
          egress::hdr.test<32> ^0 [0:31] ]
    rotational clusters:
        [[ingress::hdr.test<32> ^0 [0:19]], [ingress::hdr.test<32> ^4 [20:31]], [egress::hdr.test<32> ^0 [0:31]]]
)");

    PhvInfo phv;
    std::list<PHV::SuperCluster *> superclusters;
    ConstrainedFieldMap fields;
    SuperClusterBuilder scb;

    void SetUp() override {
        // Setup fields
        phv.add("ingress::hdr.test"_cs, INGRESS, 32, 0, false, false);
        phv.add("egress::hdr.test"_cs, EGRESS, 32, 0, false, false);
    }
};

TEST_F(ConstrainedFieldMapBuilderTest, ShouldInitializeFields) {
    // Compute map
    fields = ConstrainedFieldMapBuilder::buildMap(phv, superclusters);

    // Assertions
    ASSERT_FALSE(fields.find("ingress::hdr.test"_cs) == fields.end());
    ASSERT_FALSE(fields.find("egress::hdr.test"_cs) == fields.end());
}

TEST_F(ConstrainedFieldMapBuilderTest, ShouldInitializeSlices) {
    // Setup superclusters
    std::optional<PHV::SuperCluster *> sc1 = scb.build_super_cluster(SUPERCLUSTER);
    if (!sc1) FAIL() << "Failed to build the supercluster!";
    superclusters.push_back(*sc1);

    // Compute map
    fields = ConstrainedFieldMapBuilder::buildMap(phv, superclusters);

    // Assertions
    auto slices1 = fields["ingress::hdr.test"_cs].getSlices();
    auto slices2 = fields["egress::hdr.test"_cs].getSlices();

    ASSERT_EQ(slices1.size(), 2U);
    ASSERT_EQ(slices2.size(), 1U);

    auto &slice0 = *slices1.begin();
    auto &slice1 = *(++slices1.begin());
    auto &slice2 = *slices2.begin();

    EXPECT_EQ(slice0.getParent().getName(), "ingress::hdr.test"_cs);
    EXPECT_EQ(slice0.getRange().lo, 0);
    EXPECT_EQ(slice0.getRange().hi, 19);
    EXPECT_EQ(slice1.getParent().getName(), "ingress::hdr.test"_cs);
    EXPECT_EQ(slice1.getRange().lo, 20);
    EXPECT_EQ(slice1.getRange().hi, 31);
    EXPECT_EQ(slice2.getParent().getName(), "egress::hdr.test"_cs);
    EXPECT_EQ(slice2.getRange().lo, 0);
    EXPECT_EQ(slice2.getRange().hi, 31);
}

TEST_F(ConstrainedFieldMapBuilderTest, ShouldInitializeFieldConstraints) {
    // Additional setup
    auto field = phv.field("ingress::hdr.test"_cs);
    field->set_solitary(1);
    field->updateAlignment(AR::PARSER, FieldAlignment(le_bitrange(4, 4)), Util::SourceInfo());

    // Compute map
    fields = ConstrainedFieldMapBuilder::buildMap(phv, superclusters);

    // Assertions
    auto &cf = fields.at("ingress::hdr.test"_cs);
    EXPECT_TRUE(cf.getSolitary().hasConstraint());
    EXPECT_TRUE(cf.getAlignment().hasConstraint());
    EXPECT_EQ(cf.getAlignment().getAlignment(), 4u);
}

TEST_F(ConstrainedFieldMapBuilderTest, ShouldInitializeSliceConstraints) {
    // Setup superclusters
    std::optional<PHV::SuperCluster *> sc1 = scb.build_super_cluster(SUPERCLUSTER);
    if (!sc1) FAIL() << "Failed to build the supercluster!";
    superclusters.push_back(*sc1);

    // Additional setup
    auto field = phv.field("ingress::hdr.test"_cs);
    field->updateAlignment(AR::PARSER, FieldAlignment(le_bitrange(4, 4)), Util::SourceInfo());

    // Compute map
    fields = ConstrainedFieldMapBuilder::buildMap(phv, superclusters);

    // Assertions
    auto slices = fields["ingress::hdr.test"_cs].getSlices();

    auto slice = *(++slices.begin());
    EXPECT_TRUE(slice.getAlignment().hasConstraint());
    EXPECT_EQ(slice.getAlignment().getAlignment(), 4U);
}

}  // namespace P4::Test
