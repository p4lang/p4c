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

#include <gmock/gmock-matchers.h>
#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>

#include <sstream>

#include "bf-p4c/ir/gress.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/test/gtest/tofino_gtest_utils.h"

namespace P4::Test {

// Tests in this class are intended to test the correctness of the container action
// part of ActionAnalysis. Namely, given a PHV allocation for all the fields in
// an Action (including all destinations and source operands), is it possible to
// synthesize ALU instructions for it? Or in other words, is the PHV allocation
// correct to be implemented as ALU instructions?

class ContainerAction : public BackendTest {
 protected:
    ContainerAction() {
        tbl = new IR::MAU::Table("test"_cs, INGRESS);
        act = new IR::MAU::Action("act"_cs);
        tbl->actions["act"_cs] = act;
    }

    PhvInfo phv;
    IR::MAU::Table *tbl;
    IR::MAU::Action *act;

    void add_set(const PHV::Field *dst, const PHV::Field *src) {
        auto set = new IR::MAU::Instruction("set"_cs);

        set->operands.push_back(new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name));
        set->operands.push_back(new IR::TempVar(IR::Type::Bits::get(src->size), false, src->name));

        act->action.push_back(set);
    }

    void add_set(const PHV::Field *dst, unsigned src) {
        auto set = new IR::MAU::Instruction("set"_cs);

        set->operands.push_back(new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name));
        set->operands.push_back(new IR::Constant(IR::Type::Bits::get(dst->size), src));

        act->action.push_back(set);
    }

    void add_set(const PHV::Field *dst, cstring param) {
        auto set = new IR::MAU::Instruction("set"_cs);

        set->operands.push_back(new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name));
        set->operands.push_back(
            new IR::MAU::ActionArg(IR::Type::Bits::get(dst->size), "act"_cs, param));

        act->action.push_back(set);
    }

    void add_set_from_meter(const PHV::Field *dst, cstring meter) {
        auto set = new IR::MAU::Instruction("set"_cs);

        set->operands.push_back(new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name));
        set->operands.push_back(new IR::Slice(
            new IR::MAU::AttachedOutput(IR::Type::Bits::get(dst->size), new IR::MAU::Meter(meter)),
            dst->size - 1, 0));

        act->action.push_back(set);
    }

    void add_set_from_meter(const PHV::Field *dst, int slice_hi, int slice_lo, cstring meter) {
        auto set = new IR::MAU::Instruction("set"_cs);

        set->operands.push_back(new IR::Slice(
            new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name), slice_hi, slice_lo));
        set->operands.push_back(
            new IR::Slice(new IR::MAU::AttachedOutput(IR::Type::Bits::get(slice_hi - slice_lo + 1),
                                                      new IR::MAU::Meter(meter)),
                          slice_hi - slice_lo, 0));

        act->action.push_back(set);
    }

    void add_op(const PHV::Field *dst, cstring op, const PHV::Field *src1, const PHV::Field *src2) {
        auto instr = new IR::MAU::Instruction(op);

        instr->operands.push_back(
            new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name));
        instr->operands.push_back(
            new IR::TempVar(IR::Type::Bits::get(src1->size), false, src1->name));
        instr->operands.push_back(
            new IR::TempVar(IR::Type::Bits::get(src2->size), false, src2->name));

        act->action.push_back(instr);
    }

    void add_op(const PHV::Field *dst, cstring op, const PHV::Field *src1, cstring src2) {
        auto instr = new IR::MAU::Instruction(op);

        instr->operands.push_back(
            new IR::TempVar(IR::Type::Bits::get(dst->size), false, dst->name));
        instr->operands.push_back(
            new IR::TempVar(IR::Type::Bits::get(src1->size), false, src1->name));
        instr->operands.push_back(
            new IR::MAU::ActionArg(IR::Type::Bits::get(dst->size), "act"_cs, src2));

        act->action.push_back(instr);
    }

    struct Result {
        bool warn = false, error = false;
        Result(bool w, bool e) : warn(w), error(e) {}
    };

    Result analyze_container_actions() {
        ReductionOrInfo red_info;
        ActionAnalysis aa(phv, true, false, tbl, red_info);

        ActionAnalysis::ContainerActionsMap container_actions_map;
        aa.set_container_actions_map(&container_actions_map);
        aa.set_error_verbose();
        aa.set_verbose();

        tbl->apply(aa);

        return Result(aa.warning_found(), aa.error_found());
    }

    void alloc(PHV::Field *field, const char *container, unsigned field_lo, unsigned container_lo,
               unsigned width, int first_stage = 0, unsigned first_use = 0, int last_stage = 0,
               unsigned last_use = 0) {
        PHV::AllocSlice slice(field, container, field_lo, container_lo, width);

        if (Device::currentDevice() != Device::TOFINO) {
            slice.setLiveness({first_stage, PHV::FieldUse(first_use)},
                              {last_stage, PHV::FieldUse(last_use)});
        }

        field->add_alloc(slice);
        phv.add_container_to_field_entry(container, field);
    }

    void ok() {
        auto rv = analyze_container_actions();
        ASSERT_FALSE(rv.error);
    }

    void nok() {
        auto rv = analyze_container_actions();
        ASSERT_TRUE(rv.error);
    }
};

#define NOK_UNIMPLEMENTED(err_msg)                          \
    try {                                                   \
        auto rv = analyze_container_actions();              \
        ASSERT_FALSE(rv.error);                             \
    } catch (const Util::CompilerUnimplemented &e) {        \
        ASSERT_THAT(e.what(), testing::HasSubstr(err_msg)); \
    }

class TofinoContainerAction : public ContainerAction {
 public:
    TofinoContainerAction() { Device::init("Tofino"_cs); }
};

#define FIELD(field, size) field = phv.add(cstring(#field), INGRESS, size, 0, true, false);

TEST_F(TofinoContainerAction, sanity) {
    PHV::Field *f1, *f2;

    FIELD(f1, 6);
    FIELD(f2, 6);

    alloc(f1, "B0"_cs, 0, 0, 6);
    alloc(f2, "B2"_cs, 0, 0, 6);

    add_set(f1, f2);

    ok();
}

// Whether we can correctly synthesize
// instructions for the action depends on the allocation of f4.

class DepositFieldContigousMask : public TofinoContainerAction {
 protected:
    void SetUp() override {
        FIELD(f1, 8);
        FIELD(f2, 8);
        FIELD(f3, 16);
        FIELD(f4, 8);
        FIELD(f5, 8);

        alloc(f1, "W1"_cs, 0, 0, 8);
        alloc(f2, "W1"_cs, 0, 8, 8);
        alloc(f3, "W1"_cs, 0, 16, 16);
        alloc(f5, "W0"_cs, 0, 16, 8);

        add_set(f1, f4);
        add_set(f2, f5);
        add_set(f3, 0xbabe);
    }

    PHV::Field *f1, *f2, *f3, *f4, *f5;
};

TEST_F(DepositFieldContigousMask, test1) {
    alloc(f4, "W0"_cs, 0, 0, 8);

    nok();
}

TEST_F(DepositFieldContigousMask, test2) {
    alloc(f4, "W0"_cs, 0, 8, 8);

    ok();
}

class ByteRotateMerge : public TofinoContainerAction {
 protected:
    void SetUp() override {
        FIELD(d1, 2);
        FIELD(d_p1, 6);
        FIELD(d2, 1);
        FIELD(d3, 2);
        FIELD(d_p2, 5);

        FIELD(s2, 1);
        FIELD(s3, 2);

        d_p1->set_padding(true);
        d_p2->set_padding(true);

        alloc(d1, "W0"_cs, 0, 0, 2);
        alloc(d_p1, "W0"_cs, 0, 2, 6);
        alloc(d2, "W0"_cs, 0, 8, 1);
        alloc(d3, "W0"_cs, 0, 9, 2);
        alloc(d_p2, "W0"_cs, 0, 11, 5);

        add_set(d1, 0xabcd);
        add_set(d2, s2);
        add_set(d3, s3);
    }

    PHV::Field *d1, *d_p1, *d2, *d3, *d_p2;
    PHV::Field *s2, *s3;
};

TEST_F(ByteRotateMerge, one_const_and_one_source) {
    alloc(s2, "W1"_cs, 0, 0, 1);
    alloc(s3, "W1"_cs, 0, 1, 2);
    ok();
}

// The following tests are lifted directly from Evan's MAU onboarding slides.
// see slide 55 and onwards from link below:
// https://intel.sharepoint.com/:p:/r/sites/BXD/_layouts/15/Doc.aspx?sourcedoc=%7B81E2FE32-6FDA-4A39-872E-FCE2329561EB%7D&file=Day4a-Evan-MAU-Introduction.pptx&action=edit&mobileredirect=true

class SlidesExamples : public TofinoContainerAction {
 protected:
    void SetUp() override {
        FIELD(f1, 2);
        FIELD(f2, 2);
        FIELD(f3, 2);
        FIELD(f4, 2);
        FIELD(f5, 2);
        FIELD(f6, 2);
        FIELD(f7, 2);
        FIELD(f8, 2);
        FIELD(f9, 2);
        FIELD(fa, 2);
        FIELD(fb, 2);
        FIELD(fc, 2);

        alloc(f1, "B0"_cs, 0, 0, 2);
        alloc(f2, "B0"_cs, 0, 2, 2);
        alloc(f3, "B0"_cs, 0, 4, 2);
        alloc(f4, "B0"_cs, 0, 6, 2);
        alloc(f5, "B1"_cs, 0, 0, 2);
        alloc(f6, "B1"_cs, 0, 2, 2);
        alloc(f7, "B1"_cs, 0, 4, 2);
        alloc(f8, "B1"_cs, 0, 6, 2);
        alloc(f9, "B2"_cs, 0, 0, 2);
        alloc(fa, "B2"_cs, 0, 2, 2);
        alloc(fb, "B2"_cs, 0, 4, 2);
        alloc(fc, "B2"_cs, 0, 6, 2);
    }

    PHV::Field *f1, *f2, *f3, *f4, *f5, *f6, *f7, *f8, *f9, *fa, *fb, *fc;
};

TEST_F(SlidesExamples, a1) {
    add_set(f1, f5);

    ok();
}

TEST_F(SlidesExamples, a2) {
    add_set(f1, f8);

    ok();
}

TEST_F(SlidesExamples, a3) {
    add_set(f1, f5);
    add_set(f2, fa);

    nok();
}

TEST_F(SlidesExamples, a4) {
    add_set(f1, f5);
    add_set(f2, fa);
    add_set(f3, fb);
    add_set(f4, fc);

    ok();
}

TEST_F(SlidesExamples, a5) {
    add_set(f1, f5);
    add_set(f2, f6);
    add_set(f3, f9);
    add_set(f4, fa);

    ok();
}

TEST_F(SlidesExamples, a6) {
    add_set(f1, f5);
    add_set(f2, f6);
    add_set(f3, fc);
    add_set(f4, f9);

    ok();
}

TEST_F(SlidesExamples, a7) {
    add_set(f1, f7);
    add_set(f2, f8);
    add_set(f3, f9);
    add_set(f4, fa);

    nok();
}

TEST_F(SlidesExamples, a8) {
    add_set(f1, f5);
    add_set(f2, fa);
    add_set(f3, f7);
    add_set(f4, fc);

    nok();
}

TEST_F(SlidesExamples, a9) {
    add_set(f1, f5);
    add_set(f2, fa);
    add_set(f3, f7);
    add_set(f4, f8);

    ok();
}

TEST_F(SlidesExamples, a10) {
    add_set(f1, f5);
    add_set(f2, f6);
    add_set(f4, f8);

    ok();
}

TEST_F(SlidesExamples, a11) {
    add_set(f1, f6);
    add_set(f2, f7);
    add_set(f4, f5);

    nok();
}

TEST_F(SlidesExamples, a12) {
    add_set(f1, f5);
    add_set(f2, f5);
    add_set(f3, f7);
    add_set(f4, f8);

    ok();
}

TEST_F(SlidesExamples, b1) {
    add_set(f1, "param1"_cs);

    ok();
}

TEST_F(SlidesExamples, b2) {
    add_set(f1, "param1"_cs);
    add_set(f2, "param2"_cs);

    ok();
}

TEST_F(SlidesExamples, b3) {
    add_set(f1, "param1"_cs);
    add_set(f2, "param2"_cs);
    add_set(f3, f7);
    add_set(f4, f8);

    ok();
}

TEST_F(SlidesExamples, b4) {
    add_set(f1, "param1"_cs);
    add_set(f2, "param2"_cs);
    add_set(f3, f5);
    add_set(f4, f6);

    nok();
}

TEST_F(SlidesExamples, b5) {
    add_set(f1, "param1"_cs);
    add_set(f3, "param2"_cs);

    ok();
}

TEST_F(SlidesExamples, b6) {
    add_set(f1, "param1"_cs);
    add_set(f2, f6);
    add_set(f3, "param2"_cs);
    add_set(f4, f8);

    ok();
}

TEST_F(SlidesExamples, b7) {
    add_set(f1, "param1"_cs);
    add_set(f2, f6);
    add_set(f3, "param2"_cs);

    nok();
}

TEST_F(SlidesExamples, b8) {
    add_set(f1, "param1"_cs);
    add_set(f2, "param2"_cs);
    add_set(f3, f7);
    add_set(f4, "param3"_cs);

    ok();
}

TEST_F(SlidesExamples, b9) {
    add_set(f1, 0x2);

    ok();
}

TEST_F(SlidesExamples, b10) {
    add_set(f1, 0x2);
    add_set(f2, "param1"_cs);

    ok();
}

TEST_F(SlidesExamples, b11) {
    add_set(f1, 0x2);
    add_set(f2, "param2"_cs);
    add_set(f3, f7);
    add_set(f4, f8);

    ok();
}

TEST_F(SlidesExamples, c1) {
    add_op(f1, "bor"_cs, f1, f5);

    nok();
}

TEST_F(SlidesExamples, c2) {
    add_op(f1, "bor"_cs, f1, f5);
    add_op(f2, "bor"_cs, f2, f6);
    add_op(f3, "bor"_cs, f3, f7);
    add_op(f4, "bor"_cs, f4, f8);

    ok();
}

TEST_F(SlidesExamples, c3) {
    add_op(f1, "bor"_cs, f5, f9);
    add_op(f2, "bor"_cs, f6, fa);
    add_op(f3, "bor"_cs, f7, fb);
    add_op(f4, "bor"_cs, f8, fc);

    ok();
}

TEST_F(SlidesExamples, c4) {
    add_op(f1, "bor"_cs, f1, f5);
    add_op(f2, "bor"_cs, f2, f6);
    add_set(f3, f3);
    add_set(f4, f4);

    nok();
}

TEST_F(SlidesExamples, c5) {
    add_op(f1, "bor"_cs, f1, f5);
    add_op(f2, "bor"_cs, f2, f6);
    add_op(f3, "bor"_cs, f3, "param1"_cs);
    add_op(f4, "bor"_cs, f4, "param2"_cs);

    nok();
}

TEST_F(SlidesExamples, c6) {
    add_op(f1, "bor"_cs, f1, "param1"_cs);
    add_op(f2, "bor"_cs, f2, "param2"_cs);

    nok();
    // TODO this can be supported if compiler can pad zero param1 and param2
    // to byte alignment on the action RAM
}

// Test whether an action use multiple action parameters if
// all action parameters are from the same meter.
//
// A single 8b field is used, but the allocation is split
// into 2 x 4b pieces.
class MultipleActionFromMeter : public TofinoContainerAction {
 protected:
    void SetUp() override {
        FIELD(f1, 8);

        alloc(f1, "B0"_cs, 0, 0, 4);
        alloc(f1, "B0"_cs, 4, 4, 4);
    }

    PHV::Field *f1;
};

// Test 1: Regular set from a parameter
// Expectation: OK -- no speciality
TEST_F(MultipleActionFromMeter, test_1) {
    add_set(f1, "param1"_cs);

    ok();
}

// Test 2: Set from meter
// Expectation: OK - sourced from the same meter
TEST_F(MultipleActionFromMeter, test_2) {
    add_set_from_meter(f1, "meter1"_cs);

    ok();
}

// Test 3: Set from two meters
// Expectation: EXCEPTION -- setting from multiple speciality sources
TEST_F(MultipleActionFromMeter, test_3) {
    add_set_from_meter(f1, 3, 0, "meter1"_cs);
    add_set_from_meter(f1, 7, 4, "meter2"_cs);

    NOK_UNIMPLEMENTED("packing is too complicated");
}

class JBayContainerAction : public ContainerAction {
 public:
    static constexpr unsigned R = PHV::FieldUse::READ;
    static constexpr unsigned W = PHV::FieldUse::WRITE;

    JBayContainerAction() {
        Device::init("Tofino2"_cs);
        PhvInfo::table_to_min_stages["test"_cs] = {0};
    }

 protected:
    void alloc_src(PHV::Field *field, const char *container, unsigned container_lo) {
        alloc(field, container, 0, container_lo, field->size, 0, R, 1, W);
    }

    void alloc_dst(PHV::Field *field, const char *container, unsigned container_lo) {
        alloc(field, container, 0, container_lo, field->size, 0, W, 1, R);
    }
};

// Tests basic dark container constraint - whole container is written (no mask)
class Dark : public JBayContainerAction {
 protected:
    void SetUp() override {
        FIELD(f1, 1);
        FIELD(f2, 7);
        FIELD(f3, 1);
        FIELD(f4, 7);

        add_set(f1, f3);
    }

    PHV::Field *f1, *f2, *f3, *f4;
};

TEST_F(Dark, test1) {
    alloc_dst(f1, "DB8"_cs, 0);
    alloc_src(f2, "DB8"_cs, 1);
    alloc_src(f3, "B28"_cs, 0);

    nok();
}

TEST_F(Dark, test2) {
    alloc_dst(f1, "DB8"_cs, 0);
    alloc_src(f3, "B28"_cs, 0);

    ok();
}

TEST_F(Dark, test3) {
    alloc_dst(f1, "B0"_cs, 0);
    alloc_src(f3, "DB0"_cs, 0);

    ok();
}

TEST_F(Dark, test4) {
    alloc_dst(f1, "B0"_cs, 0);
    alloc_src(f2, "B0"_cs, 1);
    alloc_src(f3, "DB0"_cs, 0);

    ok();
}

TEST_F(Dark, test5) {
    add_set(f2, f4);

    alloc_dst(f1, "B0"_cs, 0);
    alloc_src(f2, "B0"_cs, 1);
    alloc_src(f3, "DB0"_cs, 0);
    alloc_src(f4, "DB0"_cs, 1);

    ok();
}

class Darker : public Dark {
 protected:
    void SetUp() override {
        FIELD(f1, 8);
        FIELD(f2, 8);
        FIELD(f3, 8);

        add_set(f1, f2);
    }
};

// More basic tests -- diffrent field size, container size variations.
TEST_F(Darker, test1) {
    alloc_dst(f1, "DB0"_cs, 0);
    alloc_src(f2, "B0"_cs, 0);

    ok();
}

TEST_F(Darker, test2) {
    alloc_dst(f1, "B0"_cs, 0);
    alloc_src(f2, "DB0"_cs, 0);

    ok();
}

TEST_F(Darker, test3) {
    alloc_dst(f1, "DH0"_cs, 0);
    alloc_src(f2, "H0"_cs, 0);

    ok();
}

TEST_F(Darker, test4) {
    alloc_dst(f1, "DH0"_cs, 0);
    alloc_src(f2, "H0"_cs, 0);
    alloc_dst(f3, "DH0"_cs, 8);

    nok();
}

TEST_F(Darker, test5) {
    alloc_dst(f1, "H0"_cs, 2);
    alloc_src(f2, "DH0"_cs, 0);

    ok();
}

// Dark container can always be sourced in ALU ops.
class DarkSource : public Dark {
 protected:
    void SetUp() override {
        FIELD(f1, 8);
        FIELD(f2, 8);
        FIELD(f3, 8);

        add_op(f1, "add"_cs, f2, f3);
    }
};

TEST_F(DarkSource, test1) {
    alloc_dst(f1, "B0"_cs, 0);
    alloc_src(f2, "DB0"_cs, 0);
    alloc_src(f3, "DB1"_cs, 0);

    ok();
}

TEST_F(DarkSource, test2) {
    alloc_dst(f1, "H0"_cs, 0);
    alloc_src(f2, "DH0"_cs, 0);
    alloc_src(f3, "DH1"_cs, 0);

    ok();
}

class DarkCannotSourceActionRAM : public JBayContainerAction {
 protected:
    void SetUp() override { FIELD(f1, 8); }

    PHV::Field *f1;
};

TEST_F(DarkCannotSourceActionRAM, test1) {
    add_set(f1, 0xbabe);

    alloc_dst(f1, "DB0"_cs, 0);

    nok();
}

TEST_F(DarkCannotSourceActionRAM, test2) {
    add_set(f1, "param"_cs);

    alloc_dst(f1, "DB0"_cs, 0);

    nok();
}

TEST_F(DarkCannotSourceActionRAM, test3) {
    add_set(f1, 0xbabe);

    alloc_dst(f1, "MB0"_cs, 0);

    ok();
}

TEST_F(DarkCannotSourceActionRAM, test4) {
    add_set(f1, "param"_cs);

    alloc_dst(f1, "MB0"_cs, 0);

    ok();
}

}  // namespace P4::Test
