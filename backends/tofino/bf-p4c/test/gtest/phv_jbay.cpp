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

#include "bf-p4c/phv/phv.h"
#include "gtest/gtest.h"
#include "ir/ir.h"
#include "ir/json_loader.h"
#include "lib/bitvec.h"
#include "lib/error.h"
#include "test/gtest/helpers.h"
#include "tofino_gtest_utils.h"

namespace P4::Test {

namespace {

// Test that each type of PHV::Container has the expected properties.
void CheckJBayPhvContainerTypes() {
    using Type = PHV::Type;
    const auto &phvSpec = Device::phvSpec();

    auto checkRange = [&](PHV::Container c) {
        SCOPED_TRACE(c);
        auto begin = c.index() / 2;
        auto length = std::max(c.index() * 2, 128u);
        for (auto type : phvSpec.containerTypes()) {
            auto range = phvSpec.range(type, begin, length);
            if (type == c.type())
                EXPECT_TRUE(range.getbit(phvSpec.containerToId(c)));
            else
                EXPECT_FALSE(range.getbit(phvSpec.containerToId(c)));
        }

        auto rangeBefore = phvSpec.range(c.type(), 0, c.index());
        EXPECT_FALSE(rangeBefore.getbit(phvSpec.containerToId(c)));
        auto rangeAfter = phvSpec.range(c.type(), c.index() + 1, 64);
        EXPECT_FALSE(rangeAfter.getbit(phvSpec.containerToId(c)));
    };

    PHV::Container c;
    EXPECT_FALSE(static_cast<bool>(c));

    c = "B0";
    EXPECT_TRUE(static_cast<bool>(c));
    EXPECT_EQ(Type("B"), c.type());
    EXPECT_EQ(0u, c.log2sz());
    EXPECT_EQ(0u, c.index());
    EXPECT_FALSE(c.is(PHV::Kind::tagalong));
    EXPECT_EQ(c, PHV::Container(Type("B"), 0));
    EXPECT_EQ(c, phvSpec.idToContainer(phvSpec.containerToId(c)));
    EXPECT_TRUE(phvSpec.range(Type("B"), 0, 16).getbit(phvSpec.containerToId(c)));
    EXPECT_TRUE(phvSpec.range(Type("B"), 0, 16).getbit(phvSpec.containerToId(c)));
    checkRange(c);

    c = "H15";
    EXPECT_TRUE(static_cast<bool>(c));
    EXPECT_EQ(Type("H"), c.type());
    EXPECT_EQ(1u, c.log2sz());
    EXPECT_EQ(15u, c.index());
    EXPECT_FALSE(c.is(PHV::Kind::tagalong));
    EXPECT_EQ(c, PHV::Container(Type("H"), 15));
    EXPECT_EQ(c, phvSpec.idToContainer(phvSpec.containerToId(c)));
    checkRange(c);

    c = "W3157";
    EXPECT_TRUE(static_cast<bool>(c));
    EXPECT_EQ(Type("W"), c.type());
    EXPECT_EQ(2u, c.log2sz());
    EXPECT_EQ(3157u, c.index());
    EXPECT_FALSE(c.is(PHV::Kind::tagalong));
    EXPECT_EQ(c, PHV::Container(Type("W"), 3157));
    EXPECT_EQ(c, phvSpec.idToContainer(phvSpec.containerToId(c)));
    checkRange(c);

    c = "MW31";
    EXPECT_TRUE(static_cast<bool>(c));
    EXPECT_EQ(Type("MW"), c.type());
    EXPECT_EQ(2u, c.log2sz());
    EXPECT_EQ(31u, c.index());
    EXPECT_EQ(c, PHV::Container(Type("MW"), 31));
    EXPECT_EQ(c, phvSpec.idToContainer(phvSpec.containerToId(c)));
    checkRange(c);

    c = "DB0";
    EXPECT_TRUE(static_cast<bool>(c));
    EXPECT_EQ(Type("DB"), c.type());
    EXPECT_EQ(0u, c.log2sz());
    EXPECT_EQ(0u, c.index());
    EXPECT_EQ(c, PHV::Container(Type("DB"), 0));
    EXPECT_EQ(c, phvSpec.idToContainer(phvSpec.containerToId(c)));
    checkRange(c);

    EXPECT_ANY_THROW(PHV::Container("X1"));
    EXPECT_ANY_THROW(PHV::Container("B"));  // TODO
    EXPECT_ANY_THROW(PHV::Container("W-1"));
    EXPECT_ANY_THROW(PHV::Container("MD1"));
    EXPECT_ANY_THROW(PHV::Container("MDB1"));
    EXPECT_ANY_THROW(PHV::Container("BD1"));
    EXPECT_ANY_THROW(PHV::Container("M9"));
    EXPECT_ANY_THROW(PHV::Container("D666"));
}

void CheckJBayPhvContainerResources() {
    const auto &phvSpec = Device::phvSpec();

    // MAU containers should be subsets of the physical containers.
    for (auto s : phvSpec.containerSizes()) {
        for (auto mau_group : phvSpec.mauGroups(s)) {
            EXPECT_NE(bitvec(), mau_group & phvSpec.physicalContainers());
        }
    }

    // There should be 4 MAU groups of size b8, 6 of b16, and 4 of b32.
    // Each group has 20 containers: 12 normal, 4 dark, and 4 mocha.
    ordered_map<PHV::Size, int> mau_group_sizes;
    const std::map<PHV::Size, std::set<PHV::Type>> groupsToTypes = phvSpec.groupsToTypes();
    for (auto s : phvSpec.containerSizes()) {
        for (auto containers : phvSpec.mauGroups(s)) {
            // MAU groups should not be empty.
            EXPECT_LE((unsigned)0, containers.min());
            // Each MAU group should have an entry in the groupsToTypes map.
            EXPECT_EQ((unsigned)1, groupsToTypes.count(s));
            // For JBay, each group should have three types associated with it: Dark, Mocha, and
            // Normal.
            EXPECT_EQ((unsigned)3, groupsToTypes.at(s).size());
            std::map<PHV::Kind, int> typeNum;
            for (auto cid : containers) {
                auto t = phvSpec.idToContainer(cid).type();
                typeNum[t.kind()]++;
            }
            // Check the number of containers of each type in each MAU group.
            EXPECT_EQ(12, typeNum[PHV::Kind::normal]);
            EXPECT_EQ(4, typeNum[PHV::Kind::mocha]);
            EXPECT_EQ(4, typeNum[PHV::Kind::dark]);
            mau_group_sizes[s]++;
        }
    }

    EXPECT_EQ(4, mau_group_sizes[PHV::Size::b8]);
    EXPECT_EQ(6, mau_group_sizes[PHV::Size::b16]);
    EXPECT_EQ(4, mau_group_sizes[PHV::Size::b32]);
}

// Test that we can serialize PHV::Container objects to JSON.
void CheckJBayPhvContainerJSON() {
    std::vector<PHV::Container> inputs = {
        PHV::Container("B0"),  PHV::Container("H33"), PHV::Container("W55"),
        PHV::Container("MW5"), PHV::Container("DB6"),
    };

    for (PHV::Container inputContainer : inputs) {
        SCOPED_TRACE(inputContainer);

        // Serialize the container to JSON and deserialize it back again.
        std::stringstream jsonStream;
        P4::JSONGenerator generator(jsonStream);
        generator << inputContainer;
        P4::JSONLoader loader(jsonStream);
        PHV::Container outputContainer;
        loader >> outputContainer;

        // Make sure it survived the round trip unscathed.
        EXPECT_EQ(inputContainer, outputContainer);
    }
}

}  // namespace

class JBayPhvContainer : public JBayBackendTest {};

TEST_F(JBayPhvContainer, Types) {
    EXPECT_EQ(Device::JBAY, Device::currentDevice());
    CheckJBayPhvContainerTypes();
}

TEST_F(JBayPhvContainer, Resources) {
    EXPECT_EQ(Device::JBAY, Device::currentDevice());
    CheckJBayPhvContainerResources();
}

TEST_F(JBayPhvContainer, JSON) {
    EXPECT_EQ(Device::JBAY, Device::currentDevice());
    CheckJBayPhvContainerJSON();
}

}  // namespace P4::Test
