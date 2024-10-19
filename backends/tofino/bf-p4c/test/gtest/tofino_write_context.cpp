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

#include "bf-p4c/ir/tofino_write_context.h"

#include "gtest/gtest.h"
#include "ir/ir.h"
#include "test/gtest/helpers.h"

namespace P4::Test {

namespace {

static IR::Constant *zero = new IR::Constant(0);
static IR::Constant *one = new IR::Constant(1);
static IR::Constant *two = new IR::Constant(2);
static IR::This *thisVal = new IR::This();

class TestRead : public Inspector, TofinoWriteContext {
    bool preorder(const IR::Expression *p) {
        if (findContext<IR::BFN::Select>()) {
            EXPECT_TRUE(isRead());
            EXPECT_FALSE(isWrite());
            return true;
        }
        if ((findContext<IR::MAU::Instruction>() && p == one) ||
            (findContext<IR::MAU::TypedPrimitive>() && p == zero)) {
            EXPECT_TRUE(isRead());
            EXPECT_FALSE(isWrite());
            return true;
        }
        if (findContext<IR::MAU::TypedPrimitive>() && p == two) {
            EXPECT_TRUE(isRead());
            EXPECT_TRUE(isWrite());
            return true;
        }
        return true;
    }

 public:
    TestRead() {}
};

}  // namespace

TEST(TofinoWriteContext, Read) {
    auto *zeroLVal = new IR::Member(zero, "zero");
    auto *oneLVal = new IR::Member(one, "one");

    IR::Vector<IR::BFN::ParserPrimitive> statements = {
        new IR::BFN::Extract(zeroLVal, new IR::BFN::PacketRVal(StartLen(0, 1), false)),
        new IR::BFN::Extract(oneLVal, new IR::BFN::PacketRVal(StartLen(1, 2), false)),
        new IR::BFN::Extract(zeroLVal, new IR::BFN::MetadataRVal(StartLen(256, 1)))};
    auto *state = new IR::BFN::ParserState("foo"_cs, INGRESS, statements, {}, {});

    state->apply(TestRead());

    auto prim = new IR::MAU::TypedPrimitive("foo_prim"_cs);
    prim->operands = IR::Vector<IR::Expression>({thisVal, zero, one, two});
    prim->method_type = new IR::Type_Method(
        new IR::ParameterList(IR::IndexedVector<IR::Parameter>(
            {new IR::Parameter(IR::ID("zero"), IR::Direction::In, IR::Type_Bits::get(8)),
             new IR::Parameter(IR::ID("one"), IR::Direction::Out, IR::Type_Bits::get(8)),
             new IR::Parameter(IR::ID("two"), IR::Direction::InOut, IR::Type_Bits::get(8))})),
        "foo"_cs);
    prim->apply(TestRead());

    auto *inst = new IR::MAU::Instruction("foo_inst"_cs);
    inst->operands = IR::Vector<IR::Expression>({zero, one});
    inst->apply(TestRead());
}

class TestWrite : public Inspector, TofinoWriteContext {
    bool preorder(const IR::Expression *p) {
        if ((findContext<IR::MAU::SaluAction>() && p == one) ||
            (findContext<IR::MAU::Instruction>() && p == zero) ||
            (findContext<IR::MAU::TypedPrimitive>() && p == one)) {
            EXPECT_FALSE(isRead());
            EXPECT_TRUE(isWrite());
        }
        if (findContext<IR::MAU::TypedPrimitive>() && p == two) {
            EXPECT_TRUE(isRead());
            EXPECT_TRUE(isWrite());
        }
        return true;
    }

 public:
    TestWrite() {}
};

TEST(TofinoWriteContext, Write) {
    auto *salu = new IR::MAU::SaluAction(IR::ID("foo"));
    salu->output_dst = one;

    salu->apply(TestWrite());

    auto prim = new IR::MAU::TypedPrimitive("foo_prim"_cs);
    prim->operands = IR::Vector<IR::Expression>({thisVal, zero, one, two});
    prim->method_type = new IR::Type_Method(
        new IR::ParameterList(IR::IndexedVector<IR::Parameter>(
            {new IR::Parameter(IR::ID("zero"), IR::Direction::In, IR::Type_Bits::get(8)),
             new IR::Parameter(IR::ID("one"), IR::Direction::Out, IR::Type_Bits::get(8)),
             new IR::Parameter(IR::ID("two"), IR::Direction::InOut, IR::Type_Bits::get(8))})),
        "foo"_cs);
    prim->apply(TestWrite());

    auto *inst = new IR::MAU::Instruction("foo_inst"_cs);
    inst->operands = IR::Vector<IR::Expression>({zero, one});
    inst->apply(TestWrite());
}

TEST(TofinoWriteContext, DeparserEmit) {
    auto *header = new IR::Header("foo", new IR::Type_Header("foo_t"));
    auto *field = new IR::Member(new IR::ConcreteHeaderRef(header), "bar");
    auto *povBit = new IR::Member(new IR::ConcreteHeaderRef(header), "$valid");

    auto *deparser = new IR::BFN::Deparser(EGRESS);
    deparser->emits.push_back(new IR::BFN::EmitField(field, povBit));

    struct CheckEmit : public Inspector, TofinoWriteContext {
        bool preorder(const IR::Member *) override {
            EXPECT_TRUE(isRead());
            EXPECT_FALSE(isWrite());
            return true;
        }
    };
    deparser->apply(CheckEmit());
}

TEST(TofinoWriteContext, DeparserEmitChecksum) {
    auto *header = new IR::Header("foo", new IR::Type_Header("foo_t"));
    auto *field = new IR::Member(new IR::ConcreteHeaderRef(header), "bar");
    auto *csum = new IR::Member(new IR::ConcreteHeaderRef(header), "csum");
    auto *povBit = new IR::Member(new IR::ConcreteHeaderRef(header), "$valid");

    auto *deparser = new IR::BFN::Deparser(EGRESS);
    deparser->emits.push_back(new IR::BFN::EmitChecksum(
        new IR::BFN::FieldLVal(povBit),
        {new IR::BFN::ChecksumEntry(new IR::BFN::FieldLVal(field), new IR::BFN::FieldLVal(povBit),
                                    field->type->width_bits())},
        new IR::BFN::ChecksumLVal(csum)));

    struct CheckEmitChecksum : public Inspector, TofinoWriteContext {
        bool preorder(const IR::Member *) override {
            EXPECT_TRUE(isRead());
            EXPECT_FALSE(isWrite());
            return true;
        }
    };
    deparser->apply(CheckEmitChecksum());
}

}  // namespace P4::Test
