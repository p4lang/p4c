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

#include <sstream>

#include "bf-p4c/device.h"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"
#include "lib/bitvec.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

class TofinoField : public TofinoBackendTest {};

TEST_F(TofinoField, foreach_byte) {
    const PhvSpec &phvSpec = Device::phvSpec();
    std::vector<PHV::AllocSlice> expected_slices;
    int count;

    std::map<PHV::Type, std::vector<PHV::Container>> containers;
    for (auto cid : phvSpec.physicalContainers()) {
        if (phvSpec.ingressOnly()[cid] || phvSpec.egressOnly()[cid]) continue;
        auto c = phvSpec.idToContainer(cid);
        containers[c.type()].push_back(c);
    }

    PHV::Container c8 = containers[PHV::Type::B][0];
    PHV::Container c16 = containers[PHV::Type::H][0];

    // Make a field.
    auto *f = new PHV::Field();
    std::stringstream ss;
    f->id = 16;
    f->size = 16;
    ss << "f" << f->id;
    f->name = ss.str();
    f->gress = INGRESS;
    f->validContainerRange_i = ZeroToMax();
    f->alignment = std::nullopt;
    f->set_exact_containers(true);

    // Simple allocation to one container.
    f->set_alloc({// Field MSB-->LSB.
                  PHV::AllocSlice(f, c16, 0, 0, 16)});

    expected_slices = {// Field LSB-->MSB (opposite alloc_i).
                       PHV::AllocSlice(f, c16, 0, 0, 8), PHV::AllocSlice(f, c16, 8, 8, 8)};

    count = 0;
    f->foreach_byte(StartLen(0, 16), static_cast<PHV::AllocContext *>(nullptr), nullptr,
                    [&](const PHV::AllocSlice &slice) {
                        EXPECT_EQ(expected_slices[count], slice);
                        count++;
                    });

    // Simple allocation to one container with limited range.
    f->set_alloc({// Field MSB-->LSB.
                  PHV::AllocSlice(f, c16, 0, 0, 16)});

    expected_slices = {// Field LSB-->MSB (opposite alloc_i).
                       PHV::AllocSlice(f, c16, 1, 1, 7), PHV::AllocSlice(f, c16, 8, 8, 7)};

    count = 0;
    f->foreach_byte(FromTo(1, 14), static_cast<PHV::AllocContext *>(nullptr), nullptr,
                    [&](const PHV::AllocSlice &slice) {
                        EXPECT_EQ(expected_slices[count], slice);
                        count++;
                    });

    // Simple allocation to two containers.
    f->set_alloc({
        // Field MSB-->LSB.
        PHV::AllocSlice(f, c8, 8, 0, 8),
        PHV::AllocSlice(f, c16, 0, 0, 8),
    });

    expected_slices = {// Field LSB-->MSB (opposite alloc_i).
                       PHV::AllocSlice(f, c16, 0, 0, 8), PHV::AllocSlice(f, c8, 8, 0, 8)};

    count = 0;
    f->foreach_byte(StartLen(0, 16), static_cast<PHV::AllocContext *>(nullptr), nullptr,
                    [&](const PHV::AllocSlice &slice) {
                        EXPECT_EQ(expected_slices[count], slice);
                        count++;
                    });

    // Simple allocation to two containers, but the allocation to c16 spans two
    // container bytes.
    f->set_alloc({
        // Field MSB-->LSB.
        PHV::AllocSlice(f, c8, 8, 0, 8),
        PHV::AllocSlice(f, c16, 0, 4, 8),
    });

    expected_slices = {// Field LSB-->MSB (opposite alloc_i).
                       PHV::AllocSlice(f, c16, 0, 4, 4), PHV::AllocSlice(f, c16, 4, 8, 4),
                       PHV::AllocSlice(f, c8, 8, 0, 8)};

    count = 0;
    f->foreach_byte(StartLen(0, 16), static_cast<PHV::AllocContext *>(nullptr), nullptr,
                    [&](const PHV::AllocSlice &slice) {
                        EXPECT_EQ(expected_slices[count], slice);
                        count++;
                    });

    // Test a corner case that triggered a bug in foreach_byte on switch_dc_basic.
    f->set_alloc({// Field MSB-->LSB.
                  PHV::AllocSlice(f, c8, 11, 0, 5), PHV::AllocSlice(f, c8, 10, 7, 1),
                  PHV::AllocSlice(f, c16, 0, 0, 10)});

    expected_slices = {
        // Field LSB-->MSB (opposite alloc_i).
        PHV::AllocSlice(f, c16, 1, 1, 7),
        PHV::AllocSlice(f, c16, 8, 8, 2),
        PHV::AllocSlice(f, c8, 10, 7, 1),
        PHV::AllocSlice(f, c8, 11, 0, 4),
    };

    count = 0;
    f->foreach_byte(FromTo(1, 14), static_cast<PHV::AllocContext *>(nullptr), nullptr,
                    [&](const PHV::AllocSlice &slice) {
                        EXPECT_EQ(expected_slices[count], slice);
                        count++;
                    });
}

TEST_F(TofinoField, byteAlignedRangeInBits) {
    // Make a field.
    auto *f = new PHV::Field();
    f->offset = 0;
    f->size = 1;

    EXPECT_EQ(f->byteAlignedRangeInBits(), le_bitrange(StartLen(0, 8)));

    f->offset = 0;
    f->size = 2;

    EXPECT_EQ(f->byteAlignedRangeInBits(), le_bitrange(StartLen(0, 8)));

    f->offset = 0;
    f->size = 4;

    EXPECT_EQ(f->byteAlignedRangeInBits(), le_bitrange(StartLen(0, 8)));

    f->offset = 4;
    f->size = 1;

    EXPECT_EQ(f->byteAlignedRangeInBits(), le_bitrange(StartLen(0, 8)));

    f->offset = 4;
    f->size = 3;

    EXPECT_EQ(f->byteAlignedRangeInBits(), le_bitrange(StartLen(0, 8)));

    f->offset = 4;
    f->size = 4;

    EXPECT_EQ(f->byteAlignedRangeInBits(), le_bitrange(StartLen(0, 8)));
}

class TofinoFieldSlice : public TofinoBackendTest {};

// If the size of a field's valid_container_range is smaller than field's size,
// then the fieldSlice created from the field should have a conservative valid
// container range, [0...size-1].
TEST_F(TofinoFieldSlice, field_size_lt_valid_range) {
    // mock a field.
    auto *f = new PHV::Field();
    std::stringstream ss;
    f->id = 16;
    f->size = 32;
    ss << "f" << f->id;
    f->name = ss.str();
    f->gress = INGRESS;
    f->validContainerRange_i = StartLen(0, 24);
    f->alignment = FieldAlignment(le_bitrange(StartLen(3, 32)));
    f->set_parsed(true);

    auto fs1 = new PHV::FieldSlice(f, StartLen(0, 24));
    auto fs2 = new PHV::FieldSlice(f, StartLen(24, 8));
    EXPECT_EQ(nw_bitrange(StartLen(0, 24)), fs1->validContainerRange());
    EXPECT_EQ(nw_bitrange(StartLen(0, 8)), fs2->validContainerRange());
}

}  // namespace P4::Test
