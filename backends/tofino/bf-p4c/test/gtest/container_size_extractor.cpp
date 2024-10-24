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

#include "bf-p4c/logging/container_size_extractor.h"

#include <iostream>
#include <list>
#include <sstream>

#include "bf-p4c/test/gtest/tofino_gtest_utils.h"
#include "gtest/gtest.h"

namespace P4::Test {

class ContainerSizeExtractorTest : public TofinoBackendTest, public ContainerSizeExtractor {
 protected:
    ConstrainedField field = ConstrainedField("dummy"_cs);
    const std::vector<ConstrainedSlice> &slices = field.getSlices();
};

TEST_F(ContainerSizeExtractorTest, IsLoggableOnFieldWorks) {
    EXPECT_TRUE(ContainerSizeExtractor::isLoggableOnField({32}));
    EXPECT_TRUE(ContainerSizeExtractor::isLoggableOnField({8, 8, 8, 8}));
    EXPECT_FALSE(ContainerSizeExtractor::isLoggableOnField({16, 8}));
}

TEST_F(ContainerSizeExtractorTest, GetFieldSizeWorks) {
    field.addSlice(ConstrainedSlice(field, le_bitrange(0, 3)));
    field.addSlice(ConstrainedSlice(field, le_bitrange(4, 7)));
    EXPECT_EQ(ContainerSizeExtractor::getFieldSize(field), 8u);

    field.addSlice(ConstrainedSlice(field, le_bitrange(0, 7)));
    EXPECT_EQ(ContainerSizeExtractor::getFieldSize(field), 8u);
}

TEST_F(ContainerSizeExtractorTest, CanComputeSlicing) {
    // Normal layount
    auto s1 = ContainerSizeExtractor::computeSlicing(32, {8, 16, 8});

    ASSERT_EQ(s1.size(), 3u);
    EXPECT_EQ(s1[0], le_bitrange(0, 7));
    EXPECT_EQ(s1[1], le_bitrange(8, 23));
    EXPECT_EQ(s1[2], le_bitrange(24, 31));

    // Upcasted layout, trivially splittable
    auto s2 = ContainerSizeExtractor::computeSlicing(20, {16, 8});

    ASSERT_EQ(s2.size(), 2u);
    EXPECT_EQ(s2[0], le_bitrange(0, 15));
    EXPECT_EQ(s2[1], le_bitrange(16, 19));

    // Upcasted layout, non-trivially splittable
    auto s3 = ContainerSizeExtractor::computeSlicing(22, {8, 16, 8});

    ASSERT_EQ(s3.size(), 3u);
    EXPECT_EQ(s3[0], le_bitrange(0, 5));
    EXPECT_EQ(s3[1], le_bitrange(6, 15));
    EXPECT_EQ(s3[2], le_bitrange(16, 21));
}

TEST_F(ContainerSizeExtractorTest, CanUpdateFieldSlices) {
    // Will not update
    field.addSlice(ConstrainedSlice(field, le_bitrange(0, 3)));
    field.addSlice(ConstrainedSlice(field, le_bitrange(4, 7)));
    std::vector<le_bitrange> notUpdatingSlicing = {{0, 3}, {4, 7}};

    ContainerSizeExtractor::updateFieldSlicesWithSlicing(notUpdatingSlicing, field);
    ASSERT_EQ(field.getSlices().size(), 2u);

    // Will update
    std::vector<le_bitrange> updatingSlicing = {{0, 7}};
    ContainerSizeExtractor::updateFieldSlicesWithSlicing(updatingSlicing, field);
    ASSERT_EQ(field.getSlices().size(), 3u);
}

TEST_F(ContainerSizeExtractorTest, CanReturnSortedSlicePointers) {
    std::map<std::string, std::vector<le_bitrange>> rangeArrays = {
        {"Random order",
         {le_bitrange(8, 15), le_bitrange(4, 7), le_bitrange(16, 23), le_bitrange(0, 3)}},
        {"Reverse order",
         {le_bitrange(16, 23), le_bitrange(8, 15), le_bitrange(4, 7), le_bitrange(0, 3)}},
        {"Sorted order",
         {le_bitrange(8, 15), le_bitrange(4, 7), le_bitrange(16, 23), le_bitrange(0, 3)}}};

    for (auto kv : rangeArrays) {
        for (auto range : kv.second) {
            field.addSlice(ConstrainedSlice(field, range));
        }

        auto slicing = kv.second;
        std::sort(slicing.begin(), slicing.end());
        auto sorted = ContainerSizeExtractor::getSortedSlicePointers(slicing, field);

        // Assertions
        for (unsigned i = 0; i < sorted.size() - 1; i++) {
            auto range1 = sorted[i]->getRange();
            auto range2 = sorted[i + 1]->getRange();
            EXPECT_LT(range1, range2) << "Failed on " << kv.first << "(iteration " << i << ")";
        }

        field.getSlices().clear();
    }
}

TEST_F(ContainerSizeExtractorTest, CanBindConstraintToField) {
    field.addSlice(ConstrainedSlice(field, le_bitrange(0, 7)));
    field.addSlice(ConstrainedSlice(field, le_bitrange(8, 15)));
    std::vector<int> layout = {8, 8};

    ContainerSizeExtractor::applyConstraintToField(field, layout);

    auto &c = field.getContainerSize();
    EXPECT_TRUE(c.hasConstraint());
    EXPECT_EQ(c.getContainerSize(), 8u);
}

TEST_F(ContainerSizeExtractorTest, CanBindConstraintToField2) {
    field.addSlice(ConstrainedSlice(field, le_bitrange(0, 7)));
    field.addSlice(ConstrainedSlice(field, le_bitrange(8, 26)));
    std::vector<int> layout = {32};

    ContainerSizeExtractor::applyConstraintToField(field, layout);

    auto &c = field.getContainerSize();
    EXPECT_TRUE(c.hasConstraint());
    EXPECT_EQ(c.getContainerSize(), 32u);
}

TEST_F(ContainerSizeExtractorTest, CanBindConstraintToSlices) {
    // Setup
    field.addSlice(ConstrainedSlice(field, le_bitrange(0, 7)));
    field.addSlice(ConstrainedSlice(field, le_bitrange(8, 23)));
    std::vector<int> layout = {8, 16};

    // Compute
    ContainerSizeExtractor::applyConstraintToField(field, layout);

    // Assertions
    EXPECT_FALSE(field.getContainerSize().hasConstraint());

    auto slices = field.getSlices();
    auto sl1 = *slices.begin();
    auto sl2 = *(++slices.begin());

    EXPECT_EQ(sl1.getRange(), le_bitrange(0, 7));
    EXPECT_EQ(sl1.getContainerSize().getContainerSize(), 8U);
    EXPECT_EQ(sl2.getRange(), le_bitrange(8, 23));
    EXPECT_EQ(sl2.getContainerSize().getContainerSize(), 16U);
}

}  // namespace P4::Test
