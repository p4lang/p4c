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

#include "bf-p4c/mau/action_format.h"
#include "bf-p4c/phv/phv.h"
#include "gtest/gtest.h"
#include "lib/log.h"

namespace P4::Test {

void BasicRotation_test1(ActionData::RamSection *ad, ActionData::Argument *arg) {
    auto rotate1 = ad->can_rotate(0, 4);
    BUG_CHECK(rotate1 != nullptr, "Rotation should be possible");

    ActionData::ParameterPositions param_positions = rotate1->parameter_positions();

    EXPECT_EQ(param_positions.size(), 1U);
    auto param_pos = param_positions.find(4);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg->equiv_value(param_pos->second));
    }

    auto right_rotate1 = rotate1->can_rotate(4, 2);
    param_positions = right_rotate1->parameter_positions();
    EXPECT_EQ(param_positions.size(), 1U);
    param_pos = param_positions.find(2);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg->equiv_value(param_pos->second));
    }
}

void BasicRotation_test2(ActionData::RamSection *ad, ActionData::Argument *arg) {
    auto rotate2 = ad->can_rotate(0, 6);
    BUG_CHECK(rotate2 != nullptr, "Rotation should be possible");

    ActionData::ParameterPositions param_positions = rotate2->parameter_positions();
    EXPECT_EQ(param_positions.size(), 2U);
    auto param_pos = param_positions.find(0);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg->split(2, 3)->equiv_value(param_pos->second));
    }

    param_pos = param_positions.find(6);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg->split(0, 1)->equiv_value(param_pos->second));
    }
}

/**
 * Guarantee that the can_rotate function can work on a single level
 */
TEST(ActionFormatHelper, BasicRotation) {
    int basic_size = 8;
    safe_vector<ActionData::PackingConstraint> pc_vec;
    for (int i = 0; i < 8; i++) {
        pc_vec.emplace_back();
    }
    ActionData::PackingConstraint pc(1, pc_vec);
    ActionData::RamSection *ad = new ActionData::RamSection(basic_size, pc);
    ActionData::Argument *arg = new ActionData::Argument("arg1", {0, 3});
    ad->add_param(0, arg);
    BasicRotation_test1(ad, arg);
    BasicRotation_test2(ad, arg);
}

void LayeredRotation_test1(ActionData::RamSection *ad, ActionData::Argument *arg1,
                           ActionData::Argument *arg2, ActionData::Argument *arg3) {
    auto byte_rotate = ad->can_rotate(8, 0);
    BUG_CHECK(byte_rotate != nullptr, "Rotation should be possible");
    ActionData::ParameterPositions param_positions = byte_rotate->parameter_positions();

    EXPECT_EQ(param_positions.size(), 3U);
    auto param_pos = param_positions.find(0);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg3->equiv_value(param_pos->second));
    }

    param_pos = param_positions.find(8);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg1->equiv_value(param_pos->second));
    }

    param_pos = param_positions.find(12);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg2->equiv_value(param_pos->second));
    }
}

void LayeredRotation_test2(ActionData::RamSection *ad, ActionData::Argument *arg1,
                           ActionData::Argument *arg2, ActionData::Argument *arg3) {
    auto bit_rotate1 = ad->can_rotate(0, 12);
    BUG_CHECK(bit_rotate1 != nullptr, "Rotation should be possible");
    ActionData::ParameterPositions param_positions = bit_rotate1->parameter_positions();

    EXPECT_EQ(param_positions.size(), 3U);
    auto param_pos = param_positions.find(0);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg3->equiv_value(param_pos->second));
    }

    param_pos = param_positions.find(8);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg2->equiv_value(param_pos->second));
    }

    param_pos = param_positions.find(12);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg1->equiv_value(param_pos->second));
    }

    auto bit_rotate2 = ad->can_rotate(12, 0);
    EXPECT_EQ(bit_rotate2, nullptr);
}

/**
 * Verifies that the can_rotate function can work through multiple levels of RecursiveConstraints
 */
TEST(ActionFormatHelper, LayeredRotation) {
    int first_layer_size = 8;
    safe_vector<ActionData::PackingConstraint> pc_vec;
    for (int i = 0; i < first_layer_size; i++) {
        pc_vec.emplace_back();
    }
    ActionData::PackingConstraint first_pc(1, pc_vec);
    int second_layer_size = 16;
    pc_vec.clear();
    pc_vec.push_back(first_pc);
    pc_vec.emplace_back();
    ActionData::PackingConstraint second_layer_pc(8, pc_vec);

    ActionData::RamSection *ad = new ActionData::RamSection(second_layer_size, second_layer_pc);

    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 3});
    ActionData::Argument *arg2 = new ActionData::Argument("arg2", {0, 3});
    ActionData::Argument *arg3 = new ActionData::Argument("arg3", {0, 7});

    ad->add_param(0, arg1);
    ad->add_param(4, arg2);
    ad->add_param(8, arg3);
    LayeredRotation_test1(ad, arg1, arg2, arg3);
    LayeredRotation_test2(ad, arg1, arg2, arg3);
}

void SimpleMerge_test1(ActionData::RamSection *ad1, ActionData::RamSection *ad2) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 3});
    ActionData::Argument *arg2 = new ActionData::Argument("arg2", {0, 3});

    ad1->add_param(0, arg1);
    ad2->add_param(4, arg2);
    auto merge = ad1->merge(ad2);
    auto pack_info = merge->get_pack_info();
    auto rec_const = pack_info.get_recursive_constraints();

    EXPECT_EQ(rec_const.size(), 8U);
    EXPECT_EQ(pack_info.get_granularity(), 1);
}

TEST(ActionFormatHelper, SimpleMerge) {
    int basic_size = 8;
    safe_vector<ActionData::PackingConstraint> pc_vec;
    for (int i = 0; i < 8; i++) {
        pc_vec.emplace_back();
    }
    ActionData::PackingConstraint pc(1, pc_vec);
    ActionData::RamSection *ad1 = new ActionData::RamSection(basic_size, pc);
    ActionData::RamSection *ad2 = new ActionData::RamSection(basic_size, pc);
    SimpleMerge_test1(ad1, ad2);
}

void OverlapMerge_test1(ActionData::RamSection *ad1, ActionData::RamSection *ad2) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 5});
    ActionData::Argument *arg2 = new ActionData::Argument("arg1", {2, 7});

    ad1->add_param(0, arg1);
    ad2->add_param(2, arg2);
    auto merge = ad1->merge(ad2);

    auto pack_info = merge->get_pack_info();
    auto rec_const = pack_info.get_recursive_constraints();

    EXPECT_EQ(rec_const.size(), 8U);
    EXPECT_EQ(pack_info.get_granularity(), 1);
}

TEST(ActionFormatHelper, OverlapMerge) {
    int basic_size = 8;
    safe_vector<ActionData::PackingConstraint> pc_vec;
    for (int i = 0; i < 8; i++) {
        pc_vec.emplace_back();
    }
    ActionData::PackingConstraint pc(1, pc_vec);
    ActionData::RamSection *ad1 = new ActionData::RamSection(basic_size, pc);
    ActionData::RamSection *ad2 = new ActionData::RamSection(basic_size, pc);
    OverlapMerge_test1(ad1, ad2);
}

void CrossSizeMerge_test1(ActionData::RamSection *ad_8bit, ActionData::RamSection *ad_16bit) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});
    ActionData::Argument *arg2 = new ActionData::Argument("arg2", {0, 7});

    ad_16bit->add_param(8, arg1);
    ad_8bit->add_param(0, arg2);
    auto merge = ad_16bit->merge(ad_8bit);

    auto pack_info = merge->get_pack_info();
    auto rec_const_level1 = pack_info.get_recursive_constraints();
    EXPECT_EQ(rec_const_level1.size(), 2U);
    EXPECT_EQ(pack_info.get_granularity(), 8);

    auto pack_info2_1 = rec_const_level1[0];
    auto rec_const_level2_1 = pack_info2_1.get_recursive_constraints();

    EXPECT_EQ(rec_const_level2_1.size(), 8U);
    EXPECT_EQ(pack_info2_1.get_granularity(), 1);

    auto pack_info3_1 = rec_const_level2_1[0];
    auto rec_const_level3_1 = pack_info3_1.get_recursive_constraints();

    EXPECT_EQ(rec_const_level3_1.size(), 0U);
    EXPECT_EQ(pack_info3_1.get_granularity(), -1);

    auto pack_info2_2 = rec_const_level1[1];
    auto rec_const_level2_2 = pack_info2_2.get_recursive_constraints();

    EXPECT_EQ(rec_const_level2_2.size(), 0U);
    EXPECT_EQ(pack_info2_2.get_granularity(), -1);
}

void CrossSizeMerge_test2(ActionData::RamSection *ad_8bit, ActionData::RamSection *ad_16bit) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 5});
    ActionData::Argument *arg2 = new ActionData::Argument("arg2", {0, 5});

    ad_8bit->add_param(0, arg1);
    ad_16bit->add_param(6, arg2);
    auto merge = ad_16bit->merge(ad_8bit);

    auto pack_info = merge->get_pack_info();
    auto rec_const_level1 = pack_info.get_recursive_constraints();
    EXPECT_EQ(rec_const_level1.size(), 2U);
    EXPECT_EQ(pack_info.get_granularity(), 8);

    auto pack_info2_1 = rec_const_level1[0];
    auto rec_const_level2_1 = pack_info2_1.get_recursive_constraints();

    EXPECT_EQ(rec_const_level2_1.size(), 0U);
    EXPECT_FALSE(pack_info2_1.is_rotational());

    auto pack_info2_2 = rec_const_level1[1];
    auto rec_const_level2_2 = pack_info2_2.get_recursive_constraints();

    EXPECT_EQ(rec_const_level2_2.size(), 0U);
    EXPECT_FALSE(pack_info2_2.is_rotational());
}

void CrossSizeMerge_test3(ActionData::RamSection *ad_8bit, ActionData::RamSection *ad_16bit) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 9});
    ActionData::Argument *arg2 = new ActionData::Argument("arg2", {0, 3});

    ad_8bit->add_param(0, arg2);
    ad_16bit->add_param(4, arg1);

    auto merge = ad_16bit->merge(ad_8bit);

    auto pack_info = merge->get_pack_info();
    auto rec_const_level1 = pack_info.get_recursive_constraints();
    EXPECT_EQ(rec_const_level1.size(), 2U);
    EXPECT_EQ(pack_info.get_granularity(), 8);

    auto pack_info2_1 = rec_const_level1[0];
    auto rec_const_level2_1 = pack_info2_1.get_recursive_constraints();

    EXPECT_EQ(rec_const_level2_1.size(), 0U);
    EXPECT_FALSE(pack_info2_1.is_rotational());

    auto pack_info2_2 = rec_const_level1[1];
    auto rec_const_level2_2 = pack_info2_2.get_recursive_constraints();

    EXPECT_EQ(rec_const_level2_2.size(), 0U);
    EXPECT_FALSE(pack_info2_2.is_rotational());
}

/**
 * Validates that the merging of the three examples in the "Action Data Packing Notes" document
 * in the Compilers Google Drive.  After their merge, the PackingConstraints are correct.
 */
TEST(ActionFormatHelper, CrossSizeMerge) {
    int layer_size1 = 8;
    int layer_size2 = 16;
    ActionData::PackingConstraint bit_pc;
    safe_vector<ActionData::PackingConstraint> pc_vec;
    ActionData::PackingConstraint layer_pc1 = bit_pc.expand(1, layer_size1);

    ActionData::PackingConstraint pc_8_bit = layer_pc1.expand(layer_size1, layer_size2);
    ActionData::RamSection *ad_8bit = new ActionData::RamSection(layer_size2, pc_8_bit);

    ActionData::PackingConstraint pc_16_bit = bit_pc.expand(1, layer_size2);
    ActionData::RamSection *ad_16bit = new ActionData::RamSection(layer_size2, pc_16_bit);

    CrossSizeMerge_test1(new ActionData::RamSection(*ad_8bit),
                         new ActionData::RamSection(*ad_16bit));
    CrossSizeMerge_test2(new ActionData::RamSection(*ad_8bit),
                         new ActionData::RamSection(*ad_16bit));
    CrossSizeMerge_test3(new ActionData::RamSection(*ad_8bit),
                         new ActionData::RamSection(*ad_16bit));
}

void RotateIntoRange_test1(ActionData::RamSection *ad, ActionData::Argument *arg) {
    auto rotate1 = ad->rotate_in_range({4, 15});
    BUG_CHECK(rotate1 != nullptr, "Rotation should be allowed");
    ActionData::ParameterPositions param_positions = rotate1->parameter_positions();

    EXPECT_EQ(param_positions.size(), 1U);
    auto param_pos = param_positions.find(4);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg->equiv_value(param_pos->second));
    }

    auto rotate2 = ad->rotate_in_range({5, 15});
    BUG_CHECK(rotate2 != nullptr, "Rotation should be allowed");

    param_positions = rotate2->parameter_positions();

    EXPECT_EQ(param_positions.size(), 1U);
    param_pos = param_positions.find(8);
    EXPECT_TRUE(param_pos != param_positions.end());
    if (param_pos != param_positions.end()) {
        EXPECT_TRUE(arg->equiv_value(param_pos->second));
    }

    auto rotate3 = ad->rotate_in_range({5, 10});
    EXPECT_EQ(rotate3, nullptr);
}

TEST(ActionFormatHelper, RotateIntoRange) {
    int first_layer_size = 8;
    safe_vector<ActionData::PackingConstraint> pc_vec;
    for (int i = 0; i < first_layer_size; i++) {
        pc_vec.emplace_back();
    }
    ActionData::PackingConstraint first_pc(1, pc_vec);
    int second_layer_size = 16;
    pc_vec.clear();
    pc_vec.push_back(first_pc);
    pc_vec.emplace_back();
    ActionData::PackingConstraint second_layer_pc(8, pc_vec);

    ActionData::RamSection *ad = new ActionData::RamSection(second_layer_size, second_layer_pc);

    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 3});
    ad->add_param(0, arg1);
    RotateIntoRange_test1(ad, arg1);
}

TEST(ActionFormatHelper, ConstantOverlap) {
    ActionData::PackingConstraint pc;
    pc = pc.expand(1, 8);
    ActionData::RamSection *constant1 = new ActionData::RamSection(8, pc);
    ActionData::RamSection *constant2 = new ActionData::RamSection(8, pc);

    ActionData::Constant *con1 = new ActionData::Constant(0xf0, 8);
    constant1->add_param(0, con1);

    ActionData::Constant *con2 = new ActionData::Constant(0xf, 4);
    constant2->add_param(0, con2);

    safe_vector<ActionData::SharedParameter> shared_params;
    constant1->gather_shared_params(constant2, shared_params, false);
    EXPECT_EQ(shared_params.size(), 1U);
    auto shared_arg = shared_params[0];
    auto con_overlap = shared_arg.param->to<ActionData::Constant>();
    EXPECT_EQ(con_overlap->value().getrange(0, 4), 0xfUL);
    EXPECT_EQ(con_overlap->size(), 4);
    EXPECT_EQ(shared_arg.a_start_bit, 4);
    EXPECT_EQ(shared_arg.b_start_bit, 0);

    ActionData::RamSection *constant3 = new ActionData::RamSection(8, pc);
    ActionData::Constant *con3 = new ActionData::Constant(0x3c, 6);
    constant3->add_param(2, con3);

    shared_params.clear();
    constant1->gather_shared_params(constant3, shared_params, false);
    EXPECT_EQ(shared_params.size(), 1U);
    shared_arg = shared_params[0];
    con_overlap = shared_arg.param->to<ActionData::Constant>();
    EXPECT_EQ(shared_arg.a_start_bit, 2);
    EXPECT_EQ(shared_arg.b_start_bit, 2);
}

TEST(ActionFormatHelper, DataSubset) {
    ActionData::PackingConstraint pc;
    pc = pc.expand(1, 8);
    ActionData::RamSection *ad1 = new ActionData::RamSection(8, pc);
    ActionData::RamSection *ad2 = new ActionData::RamSection(8, pc);

    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});
    ActionData::Argument *arg1_mini = new ActionData::Argument("arg1", {2, 5});
    ad1->add_param(0, arg1);
    ad2->add_param(2, arg1_mini);

    EXPECT_TRUE(ad2->is_data_subset_of(ad1));
    EXPECT_FALSE(ad1->is_data_subset_of(ad2));
}

void Contains_test1(ActionData::RamSection *ad_outside, ActionData::RamSection *ad_inside) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});

    ad_outside->add_param(8, arg1);
    ad_inside->add_param(0, arg1);

    int final_bit_pos = 0;
    EXPECT_TRUE(ad_outside->contains(ad_inside, 0, &final_bit_pos));
    EXPECT_EQ(final_bit_pos, 8);
    EXPECT_FALSE(ad_inside->contains(ad_outside, 0, nullptr));
    final_bit_pos = 0;
    EXPECT_TRUE(ad_outside->contains_any_rotation_from_0(ad_inside, 0, &final_bit_pos));
    EXPECT_EQ(final_bit_pos, 8);
}

void Contains_test2(ActionData::RamSection *ad_outside, ActionData::RamSection *ad_inside) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});
    ActionData::Argument *arg1_mini1 = new ActionData::Argument("arg1", {0, 3});
    ActionData::Argument *arg1_mini2 = new ActionData::Argument("arg1", {4, 7});

    ad_outside->add_param(8, arg1);
    ad_inside->add_param(0, arg1_mini2);
    ad_inside->add_param(4, arg1_mini1);

    int final_bit_pos = 0;
    EXPECT_TRUE(ad_outside->contains(ad_inside, 0, &final_bit_pos));
    EXPECT_EQ(final_bit_pos, 12);
    final_bit_pos = 16;
    // Try another rotation with a different init_bit
    EXPECT_TRUE(ad_outside->contains(ad_inside, 8, &final_bit_pos));
    final_bit_pos = 0;

    EXPECT_FALSE(ad_inside->contains(ad_outside, 0, nullptr));
    final_bit_pos = 0;
    EXPECT_TRUE(ad_outside->contains_any_rotation_from_0(ad_inside, 0, &final_bit_pos));
    EXPECT_EQ(final_bit_pos, 12);
}

void Contains_test3(ActionData::RamSection *ad_outside, ActionData::RamSection *ad_inside) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});

    ad_outside->add_param(8, arg1);
    ad_inside->add_param(4, arg1);

    EXPECT_FALSE(ad_outside->contains(ad_inside, 0, nullptr));
    EXPECT_FALSE(ad_inside->contains(ad_outside, 0, nullptr));
    EXPECT_FALSE(ad_outside->contains_any_rotation_from_0(ad_inside, 0, nullptr));
}

void Contains_test4(ActionData::RamSection *ad_outside, ActionData::RamSection *ad_inside) {
    ActionData::Constant *con1 = new ActionData::Constant(0x55, 8);
    ActionData::Constant *con2 = new ActionData::Constant(0x1, 2);

    ad_outside->add_param(8, con1);
    ad_inside->add_param(0, con2);

    EXPECT_FALSE(ad_outside->contains(ad_inside, 0, nullptr));
    EXPECT_FALSE(ad_inside->contains(ad_outside, 0, nullptr));
    int final_bit_pos = 0;
    EXPECT_TRUE(ad_outside->contains_any_rotation_from_0(ad_inside, 0, &final_bit_pos));
    EXPECT_EQ(final_bit_pos, 8);
}

void Contains_test5(ActionData::RamSection *ad_outside, ActionData::RamSection *ad_inside) {
    ActionData::Constant *con1 = new ActionData::Constant(0x55, 8);
    ActionData::Constant *con2 = new ActionData::Constant(0xfe, 8);

    ad_outside->add_param(8, con1);
    ad_inside->add_param(0, con2);

    EXPECT_FALSE(ad_outside->contains(ad_inside, 0, nullptr));
    EXPECT_FALSE(ad_inside->contains(ad_outside, 0, nullptr));
    EXPECT_FALSE(ad_outside->contains_any_rotation_from_0(ad_inside, 0, nullptr));
}

void Contains_test6(ActionData::RamSection *ad_outside, ActionData::RamSection *ad_inside) {
    ActionData::Constant *con1 = new ActionData::Constant(0x55, 8);
    ActionData::Constant *con2 = new ActionData::Constant(0xaa, 8);

    ad_outside->add_param(8, con1);
    ad_inside->add_param(0, con2);

    EXPECT_FALSE(ad_outside->contains(ad_inside, 0, nullptr));
    EXPECT_FALSE(ad_inside->contains(ad_outside, 0, nullptr));
    int final_bit_pos = 0;
    EXPECT_TRUE(ad_outside->contains_any_rotation_from_0(ad_inside, 0, &final_bit_pos));
    // Multiple possible answers for this, depends on which way the data is rotated
    EXPECT_EQ(final_bit_pos, 9);
}

TEST(ActionFormatHelper, Contains) {
    ActionData::PackingConstraint pc;
    ActionData::PackingConstraint first_layer_pc = pc.expand(1, 8);
    ActionData::PackingConstraint second_layer_pc = first_layer_pc.expand(8, 16);

    ActionData::RamSection ad_outside(16, pc);
    ActionData::RamSection ad_inside(16, second_layer_pc);

    Contains_test1(new ActionData::RamSection(ad_outside), new ActionData::RamSection(ad_inside));
    Contains_test2(new ActionData::RamSection(ad_outside), new ActionData::RamSection(ad_inside));
    Contains_test3(new ActionData::RamSection(ad_outside), new ActionData::RamSection(ad_inside));
    Contains_test4(new ActionData::RamSection(ad_outside), new ActionData::RamSection(ad_inside));
    Contains_test5(new ActionData::RamSection(ad_outside), new ActionData::RamSection(ad_inside));
    Contains_test6(new ActionData::RamSection(ad_outside), new ActionData::RamSection(ad_inside));
}

void ConditionalArgs_Test(ActionData::Parameter *arg1, ActionData::Parameter *arg1_cond1,
                          ActionData::Parameter *arg1_cond2,
                          ActionData::Parameter *arg1_extend_cond1) {
    EXPECT_FALSE(arg1->equiv_value(arg1_cond1));
    EXPECT_FALSE(arg1->equiv_value(arg1_cond2));
    EXPECT_FALSE(arg1_cond1->equiv_value(arg1_cond2));
    EXPECT_TRUE(arg1_cond1->equiv_value(arg1_cond2, false));
    ActionData::PackingConstraint pc;
    ActionData::RamSection *ram_sect1 = new ActionData::RamSection(8, pc);
    ActionData::RamSection *ram_sect2 = new ActionData::RamSection(8, pc);

    ram_sect1->add_param(0, arg1);
    ram_sect2->add_param(0, arg1_cond1);

    EXPECT_TRUE(ram_sect1->merge(ram_sect2) == nullptr);

    ActionData::RamSection *ram_sect3 = new ActionData::RamSection(16, pc);
    ram_sect3->add_param(0, arg1);
    ram_sect3->add_param(8, arg1_extend_cond1);

    auto param_positions = ram_sect3->parameter_positions();
    EXPECT_EQ(param_positions.size(), size_t(2));

    ActionData::RamSection *ram_sect4 = new ActionData::RamSection(16, pc);
    ram_sect4->add_param(0, arg1_cond1);
    ram_sect4->add_param(8, arg1_extend_cond1);

    param_positions = ram_sect4->parameter_positions();
    EXPECT_EQ(param_positions.size(), size_t(1));
}

TEST(ActionFormatHelper, ConditionalArgs) {
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});
    ActionData::Argument *arg1_cond1 = new ActionData::Argument("arg1", {0, 7});
    arg1_cond1->set_cond(ActionData::VALUE, "cond1"_cs);
    ActionData::Argument *arg1_cond2 = new ActionData::Argument("arg1", {0, 7});
    arg1_cond2->set_cond(ActionData::VALUE, "cond2"_cs);
    ActionData::Argument *arg1_extend_cond1 = new ActionData::Argument("arg1", {8, 15});
    arg1_extend_cond1->set_cond(ActionData::VALUE, "cond1"_cs);

    ConditionalArgs_Test(arg1, arg1_cond1, arg1_cond2, arg1_extend_cond1);

    ActionData::Constant *con1 = new ActionData::Constant(1, 8);
    ActionData::Constant *con1_cond1 = new ActionData::Constant(1, 8);
    con1_cond1->set_cond(ActionData::VALUE, "cond1"_cs);
    ActionData::Constant *con1_cond2 = new ActionData::Constant(1, 8);
    con1_cond2->set_cond(ActionData::VALUE, "cond2"_cs);
    ActionData::Constant *con1_extend_cond1 = new ActionData::Constant(*con1_cond1);
    ConditionalArgs_Test(con1, con1_cond1, con1_cond2, con1_extend_cond1);
}

/**
 * Test to make sure RandomNumbers in the same action cannot merge
 */
void RandomNumberSingleAction() {
    ActionData::RandomNumber *rn1_a1 = new ActionData::RandomNumber("rand1"_cs, "act1"_cs, {0, 7});
    ActionData::RandomNumber *rn2_a1 = new ActionData::RandomNumber("rand2"_cs, "act1"_cs, {0, 7});
    ActionData::RandomPadding *pad = new ActionData::RandomPadding(8);

    EXPECT_EQ(rn1_a1->size(), 8);
    EXPECT_EQ(rn2_a1->size(), 8);

    EXPECT_FALSE(rn1_a1->can_merge(rn2_a1));
    EXPECT_FALSE(rn2_a1->can_merge(rn1_a1));
    EXPECT_TRUE(rn1_a1->can_merge(pad));
    EXPECT_TRUE(pad->can_merge(rn1_a1));
    EXPECT_TRUE(rn2_a1->can_merge(pad));
    EXPECT_TRUE(pad->can_merge(rn2_a1));

    EXPECT_FALSE(rn1_a1->is_subset_of(rn2_a1));
    EXPECT_FALSE(rn2_a1->is_subset_of(rn1_a1));
    EXPECT_FALSE(rn1_a1->is_subset_of(pad));
    EXPECT_TRUE(pad->is_subset_of(rn1_a1));
    EXPECT_FALSE(rn2_a1->is_subset_of(pad));
    EXPECT_TRUE(pad->is_subset_of(rn2_a1));
}

/**
 * Test to make sure Random Numbers across actions can be merged, and their subsets work
 * appropriately
 */
void RandomNumberMultipleActions() {
    ActionData::RandomNumber *rn1_a1 = new ActionData::RandomNumber("rand1"_cs, "act1"_cs, {0, 7});
    ActionData::RandomNumber *rn2_a1 = new ActionData::RandomNumber("rand2"_cs, "act1"_cs, {0, 7});
    ActionData::RandomNumber *rn1_a2 = new ActionData::RandomNumber("rand1"_cs, "act2"_cs, {0, 7});
    ActionData::RandomNumber *rn2_a2 = new ActionData::RandomNumber("rand2"_cs, "act2"_cs, {0, 7});
    ActionData::RandomPadding *pad = new ActionData::RandomPadding(8);

    EXPECT_TRUE(rn1_a1->can_merge(rn1_a2));
    EXPECT_TRUE(rn1_a1->can_merge(rn2_a2));
    EXPECT_TRUE(rn2_a1->can_merge(rn1_a2));
    EXPECT_TRUE(rn2_a1->can_merge(rn2_a2));
    EXPECT_TRUE(rn1_a2->can_merge(rn1_a1));
    EXPECT_TRUE(rn1_a2->can_merge(rn2_a1));
    EXPECT_TRUE(rn2_a2->can_merge(rn1_a1));
    EXPECT_TRUE(rn2_a2->can_merge(rn2_a1));

    EXPECT_FALSE(rn1_a1->is_subset_of(rn1_a2));
    EXPECT_FALSE(rn1_a1->is_subset_of(rn2_a2));
    EXPECT_FALSE(rn2_a1->is_subset_of(rn1_a2));
    EXPECT_FALSE(rn2_a1->is_subset_of(rn2_a2));
    EXPECT_FALSE(rn1_a2->is_subset_of(rn1_a1));
    EXPECT_FALSE(rn1_a2->is_subset_of(rn2_a1));
    EXPECT_FALSE(rn2_a2->is_subset_of(rn1_a1));
    EXPECT_FALSE(rn2_a2->is_subset_of(rn2_a1));

    auto merge1 = rn1_a1->merge(rn2_a2);
    auto merge2 = rn2_a1->merge(rn1_a2);

    EXPECT_FALSE(merge1 == nullptr);
    EXPECT_FALSE(merge2 == nullptr);

    EXPECT_FALSE(merge1->can_merge(merge2));
    EXPECT_FALSE(merge2->can_merge(merge1));
    EXPECT_TRUE(merge1->can_merge(pad));
    EXPECT_TRUE(pad->can_merge(merge1));
    EXPECT_TRUE(merge2->can_merge(pad));
    EXPECT_TRUE(pad->can_merge(merge2));

    EXPECT_TRUE(rn1_a1->is_subset_of(merge1));
    EXPECT_TRUE(rn2_a2->is_subset_of(merge1));
    EXPECT_FALSE(rn2_a1->is_subset_of(merge1));
    EXPECT_FALSE(rn1_a2->is_subset_of(merge1));

    EXPECT_FALSE(rn1_a1->is_subset_of(merge2));
    EXPECT_FALSE(rn2_a2->is_subset_of(merge2));
    EXPECT_TRUE(rn2_a1->is_subset_of(merge2));
    EXPECT_TRUE(rn1_a2->is_subset_of(merge2));
}

/**
 * Test to make sure RandomNumbers overlap function works properly
 */
void RandomNumberOverlaps() {
    ActionData::RandomNumber *rn1_a1 = new ActionData::RandomNumber("rand1"_cs, "act1"_cs, {0, 1});
    ActionData::RandomNumber *rn2_a1 = new ActionData::RandomNumber("rand2"_cs, "act1"_cs, {0, 3});

    ActionData::RandomNumber *rn1_a2 = new ActionData::RandomNumber("rand1"_cs, "act2"_cs, {0, 5});
    ActionData::RandomNumber *rn2_a2 = new ActionData::RandomNumber("rand2"_cs, "act2"_cs, {0, 7});

    le_bitrange my_overlap = {0, 0};
    le_bitrange ad_overlap = {0, 0};

    auto overlap1 = rn1_a1->overlap(rn1_a2, false, &my_overlap, &ad_overlap);
    EXPECT_TRUE(overlap1 != nullptr);
    if (overlap1) {
        auto rn_overlap = overlap1->to<ActionData::RandomNumber>();
        EXPECT_EQ(rn_overlap->size(), 2);
        EXPECT_TRUE(my_overlap.lo == 0 && my_overlap.size() == 2);
        EXPECT_TRUE(ad_overlap.lo == 0 && ad_overlap.size() == 2);
    }

    auto overlap2 = rn2_a1->overlap(rn1_a2, false, &my_overlap, &ad_overlap);
    EXPECT_TRUE(overlap2 != nullptr);
    if (overlap2) {
        auto rn_overlap = overlap2->to<ActionData::RandomNumber>();
        EXPECT_EQ(rn_overlap->size(), 4);
        EXPECT_TRUE(my_overlap.lo == 0 && my_overlap.size() == 4);
        EXPECT_TRUE(ad_overlap.lo == 0 && ad_overlap.size() == 4);
    }

    auto overlap3 = rn1_a1->overlap(rn2_a1, false, nullptr, nullptr);
    EXPECT_EQ(overlap3, nullptr);
    auto overlap4 = rn1_a2->overlap(rn2_a2, false, nullptr, nullptr);
    EXPECT_EQ(overlap4, nullptr);
}

/**
 */
void RandomNumberRamSection() {
    ActionData::RandomNumber *rn1_a1 = new ActionData::RandomNumber("rand1"_cs, "act1"_cs, {0, 3});
    ActionData::RandomNumber *rn2_a1 = new ActionData::RandomNumber("rand1"_cs, "act1"_cs, {4, 7});
    ActionData::RandomPadding *pad = new ActionData::RandomPadding(4);
    ActionData::RandomNumber *rn1_a2 = new ActionData::RandomNumber("rand1"_cs, "act2"_cs, {0, 7});

    ActionData::PackingConstraint pc;
    ActionData::PackingConstraint first_layer_pc = pc.expand(1, 8);
    ActionData::RamSection *sect1 = new ActionData::RamSection(8, first_layer_pc);

    sect1->add_param(0, rn1_a1);
    sect1->add_param(4, pad);

    ActionData::RamSection *sect2 = new ActionData::RamSection(8, first_layer_pc);
    sect2->add_param(4, rn2_a1);
    sect2->add_param(0, pad);

    ActionData::RamSection *sect3 = new ActionData::RamSection(8, first_layer_pc);
    sect3->add_param(0, rn1_a2);

    auto sect2_rotate = sect2->can_rotate(0, 4);
    EXPECT_TRUE(sect2_rotate != nullptr);

    auto merge1 = sect1->merge(sect2);
    EXPECT_TRUE(merge1 != nullptr);
    if (merge1) {
        auto param_positions = merge1->parameter_positions();
        EXPECT_EQ(param_positions.size(), size_t(1));
        auto param_pos = param_positions.find(0);
        EXPECT_TRUE(param_pos != param_positions.end());
        if (param_pos != param_positions.end()) {
            auto rn_check = param_pos->second->to<ActionData::RandomNumber>();
            EXPECT_TRUE(rn1_a1->is_subset_of(rn_check->split(0, 3)));
            EXPECT_TRUE(pad->is_subset_of(rn_check->split(4, 7)));
            EXPECT_TRUE(rn2_a1->is_subset_of(rn_check->split(4, 7)));
            EXPECT_TRUE(pad->is_subset_of(rn_check->split(0, 3)));
        }

        int final_bit_pos = 0;
        EXPECT_TRUE(merge1->contains_any_rotation_from_0(sect1, 0, &final_bit_pos));
        EXPECT_EQ(final_bit_pos, 0);
        EXPECT_TRUE(merge1->contains_any_rotation_from_0(sect2_rotate, 0, &final_bit_pos));
        EXPECT_EQ(final_bit_pos, 4);
    }

    if (sect2_rotate) {
        auto merge2 = sect2_rotate->merge(sect1);
        EXPECT_EQ(merge2, nullptr);
    }

    auto merge3 = sect1->merge(sect3);
    EXPECT_TRUE(merge3 != nullptr);
    if (merge3) {
        auto param_positions = merge3->parameter_positions();
        EXPECT_EQ(param_positions.size(), size_t(2));
        auto param_pos = param_positions.find(0);
        if (param_pos != param_positions.end()) {
            auto rn_check = param_pos->second->to<ActionData::RandomNumber>();
            EXPECT_TRUE(rn1_a1->is_subset_of(rn_check));
        }

        param_pos = param_positions.find(4);
        EXPECT_TRUE(param_pos != param_positions.end());
        if (param_pos != param_positions.end()) {
            auto rn_check = param_pos->second->to<ActionData::RandomNumber>();
            EXPECT_TRUE(pad->is_subset_of(rn_check));
        }
    }
}

TEST(ActionFormatHelper, RandomNumberTests) {
    RandomNumberSingleAction();
    RandomNumberMultipleActions();
    RandomNumberOverlaps();
    RandomNumberRamSection();
}

void StaticEntryStandardOperation() {
    ActionData::ALUOperation alu_op(PHV::Container(PHV::Type::H, 0), ActionData::DEPOSIT_FIELD);
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 7});
    ActionData::ALUParameter ap1(arg1, {0, 7});
    ActionData::Argument *arg2 = new ActionData::Argument("arg2", {0, 7});
    ActionData::ALUParameter ap2(arg2, {8, 15});
    alu_op.add_param(ap1);
    alu_op.add_param(ap2);

    bitvec param1_value;
    param1_value.setrange(0, 2);
    param1_value.setrange(3, 3);

    bitvec static_op = alu_op.static_entry_of_arg(arg1, param1_value);
    EXPECT_EQ(static_op, param1_value);

    ActionData::Argument *arg1_split = new ActionData::Argument("arg1", {2, 7});
    static_op = alu_op.static_entry_of_arg(arg1_split, param1_value);
    EXPECT_EQ(static_op, param1_value << 2);

    bitvec param2_value;
    param2_value.setrange(1, 3);
    param2_value.setrange(4, 1);

    static_op = alu_op.static_entry_of_arg(arg2, param2_value);
    EXPECT_EQ(static_op, param2_value << 8);

    auto alu_op2 = alu_op.add_right_shift(4, nullptr);
    static_op = alu_op2->static_entry_of_arg(arg1, param1_value);
    EXPECT_EQ(static_op, bitvec(0xb003));

    static_op = alu_op2->static_entry_of_arg(arg2, param2_value);
    EXPECT_EQ(static_op, bitvec(0x01e0));

    static_op = alu_op2->static_entry_of_arg(arg1_split, param1_value);
    EXPECT_EQ(static_op, bitvec(0xc00e));
}

void StaticEntryOneParameterMultipleTimes() {
    ActionData::ALUOperation alu_op(PHV::Container(PHV::Type::H, 0), ActionData::DEPOSIT_FIELD);
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 2});
    ActionData::ALUParameter ap1(arg1, {0, 2});
    ActionData::ALUParameter ap2(arg1, {5, 7});
    ActionData::ALUParameter ap3(arg1, {11, 13});
    alu_op.add_param(ap1);
    alu_op.add_param(ap2);
    alu_op.add_param(ap3);

    bitvec param1_value(0x6);
    ActionData::Argument *arg1_split = new ActionData::Argument("arg1", {1, 1});
    EXPECT_EQ(alu_op.static_entry_of_arg(arg1, param1_value), bitvec(0x30c6));
    EXPECT_EQ(alu_op.static_entry_of_arg(arg1_split, bitvec(0x1)), bitvec(0x1042));

    auto alu_op2 = alu_op.add_right_shift(2, nullptr);
    EXPECT_EQ(alu_op2->static_entry_of_arg(arg1, param1_value), bitvec(0x8c31));
    EXPECT_EQ(alu_op2->static_entry_of_arg(arg1_split, bitvec(0x1)), bitvec(0x8410));
}

void StaticEntryConstants() {
    ActionData::ALUOperation alu_op(PHV::Container(PHV::Type::H, 0), ActionData::DEPOSIT_FIELD);
    ActionData::Constant *con1 = new ActionData::Constant(bitvec(0x56), 8);
    ActionData::Constant *con2 = new ActionData::Constant(bitvec(0x9), 4);
    ActionData::Constant *con3 = new ActionData::Constant(bitvec(0xb), 4);
    ActionData::ALUParameter ap1(con1, {4, 11});
    ActionData::ALUParameter ap2(con2, {0, 3});
    ActionData::ALUParameter ap3(con3, {12, 15});
    alu_op.add_param(ap1);
    alu_op.add_param(ap2);
    alu_op.add_param(ap3);

    EXPECT_EQ(alu_op.static_entry_of_constants(), bitvec(0xb569));

    auto alu_op2 = alu_op.add_right_shift(2, nullptr);
    EXPECT_EQ(alu_op2->static_entry_of_constants(), bitvec(0x6d5a));
}

void StaticEntryBitmaskedSet() {
    ActionData::ALUOperation alu_op(PHV::Container(PHV::Type::H, 0), ActionData::BITMASKED_SET);
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 2});
    ActionData::Constant *con1 = new ActionData::Constant(bitvec(0xf), 4);
    ActionData::ALUParameter ap1(arg1, {0, 2});
    ActionData::ALUParameter ap2(con1, {5, 9});
    ActionData::ALUParameter ap3(arg1, {11, 13});
    alu_op.add_param(ap1);
    alu_op.add_param(ap2);
    alu_op.add_param(ap3);

    ActionData::ALUParameter mask_param(new ActionData::Constant(bitvec(0x3be7), 16), {0, 15});
    alu_op.add_mask_param(mask_param);

    EXPECT_EQ(alu_op.static_entry_of_arg(arg1, bitvec(0x7)), bitvec(0x3807));
    EXPECT_EQ(alu_op.static_entry_of_constants(), bitvec(0x3be701e0));
}

void StaticEntryMaskParams() {
    ActionData::ALUOperation alu_op(PHV::Container(PHV::Type::H, 0), ActionData::BITMASKED_SET);
    ActionData::Argument *arg1 = new ActionData::Argument("arg1", {0, 2});
    ActionData::ALUParameter ap1(arg1, {0, 2});
    ActionData::ALUParameter ap2(arg1, {11, 13});

    alu_op.add_param(ap1);
    alu_op.add_param(ap2);

    ActionData::Argument *mask_param = new ActionData::Argument("cond", {0, 0});
    ActionData::ALUParameter mp1(mask_param, {0, 0});
    ActionData::ALUParameter mp2(mask_param, {1, 1});
    ActionData::ALUParameter mp3(mask_param, {2, 2});
    ActionData::ALUParameter mp4(mask_param, {11, 11});
    ActionData::ALUParameter mp5(mask_param, {12, 12});
    ActionData::ALUParameter mp6(mask_param, {13, 13});

    alu_op.add_mask_param(mp1);
    alu_op.add_mask_param(mp2);
    alu_op.add_mask_param(mp3);
    alu_op.add_mask_param(mp4);
    alu_op.add_mask_param(mp5);
    alu_op.add_mask_param(mp6);

    EXPECT_EQ(alu_op.static_entry_of_arg(arg1, bitvec(0x7)), bitvec(0x3807));
    EXPECT_EQ(alu_op.static_entry_of_arg(mask_param, bitvec(0x1)), bitvec(0x38070000));
}

TEST(ActionFormatHelper, StaticEntryTests) {
    StaticEntryStandardOperation();
    StaticEntryOneParameterMultipleTimes();
    StaticEntryConstants();
    StaticEntryBitmaskedSet();
    StaticEntryMaskParams();
}

}  // namespace P4::Test
