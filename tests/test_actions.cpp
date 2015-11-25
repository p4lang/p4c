/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <cassert>

#include "bm_sim/actions.h"

using std::string;

// some primitive definitions for testing

class SetField : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(SetField);

class Add : public ActionPrimitive<Field &, const Data &, const Data &> {
  void operator ()(Field &f, const Data &d1, const Data &d2) {
    f.add(d1, d2);
  }
};

REGISTER_PRIMITIVE(Add);

class RemoveHeader : public ActionPrimitive<Header &> {
  void operator ()(Header &hdr) {
    hdr.mark_invalid();
  }
};

REGISTER_PRIMITIVE(RemoveHeader);

class CopyHeader : public ActionPrimitive<Header &, const Header &> {
  void operator ()(Header &dst, const Header &src) {
    if(!src.is_valid()) return;
    dst.mark_valid();
    assert(dst.get_header_type_id() == src.get_header_type_id());
    for(unsigned int i = 0; i < dst.size(); i++) {
      dst[i].set(src[i]);
    }
  }
};

REGISTER_PRIMITIVE(CopyHeader);

/* this is an example of primitive that can be used to set known control
   registers, e.g. a drop() primitive that would set a drop metadata bit flag */
class CRSet : public ActionPrimitive<> {
  void operator ()() {
    get_field("test1.f16").set(666);
  }
};

REGISTER_PRIMITIVE(CRSet);

/* this is an example of primitive which uses header stacks */
class Pop : public ActionPrimitive<HeaderStack &> {
  void operator ()(HeaderStack &stack) {
    stack.pop_front();
  }
};

REGISTER_PRIMITIVE(Pop);

/* implementation of old primitive modify_field_with_hash_based_offset */
class ModifyFieldWithHashBasedOffset
  : public ActionPrimitive<Field &, const Data &,
			   const NamedCalculation &, const Data &> {
  void operator ()(Field &dst, const Data &base,
		   const NamedCalculation &hash, const Data &size) {
    uint64_t v =
      (hash.output(get_packet()) + base.get<uint64_t>()) % size.get<uint64_t>();
    dst.set(v);
  }
};

REGISTER_PRIMITIVE(ModifyFieldWithHashBasedOffset);

class ExecuteMeter : public ActionPrimitive<Field &, MeterArray &, const Data &> {
  void operator ()(Field &dst, MeterArray &meter_array, const Data &idx) {
    dst.set(meter_array.execute_meter(get_packet(), idx.get_uint()));
  }
};

REGISTER_PRIMITIVE(ExecuteMeter);

// Google Test fixture for actions tests
class ActionsTest : public ::testing::Test {
protected:

  PHVFactory phv_factory;
  std::unique_ptr<Packet> pkt;
  PHV *phv{nullptr};

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  header_id_t testHeaderS0{2}, testHeaderS1{3};

  header_stack_id_t testHeaderStack{0};

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  ActionsTest()
    : testHeaderType("test_t", 0),
      testActionFn("test_action", 0),
      testActionFnEntry(&testActionFn) {
    testHeaderType.push_back_field("f32", 32);
    testHeaderType.push_back_field("f48", 48);
    testHeaderType.push_back_field("f8", 8);
    testHeaderType.push_back_field("f16", 16);
    testHeaderType.push_back_field("f128", 128);

    phv_factory.push_back_header("test1", testHeader1, testHeaderType);
    phv_factory.push_back_header("test2", testHeader2, testHeaderType);

    phv_factory.push_back_header("testS0", testHeaderS0, testHeaderType);
    phv_factory.push_back_header("testS1", testHeaderS1, testHeaderType);
    phv_factory.push_back_header_stack("test_stack", testHeaderStack,
				       testHeaderType,
				       {testHeaderS0, testHeaderS1});
  }

  virtual void SetUp() {
    Packet::set_phv_factory(phv_factory);
    pkt = std::unique_ptr<Packet>(new Packet());
    phv = pkt->get_phv();
  }

  virtual void TearDown() {
    Packet::unset_phv_factory();
  }
};

TEST_F(ActionsTest, SetFromConst) {
  Data value(0xaba);
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ((unsigned) 0xaba, f.get_uint());
}

TEST_F(ActionsTest, SetFromActionData) {
  Data value(0xaba);
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_action_data(0);
  testActionFnEntry.push_back_action_data(value);

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ((unsigned) 0xaba, f.get_uint());
}

TEST_F(ActionsTest, SetFromField) {
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_field(testHeader1, 0); // f32

  Field &src = phv->get_field(testHeader1, 0); // 32
  src.set(0xaba);

  Field &dst = phv->get_field(testHeader1, 3); // f16
  dst.set(0);

  ASSERT_EQ((unsigned) 0, dst.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ((unsigned) 0xaba, dst.get_uint());
}

TEST_F(ActionsTest, SetFromExpression) {
  std::unique_ptr<ArithExpression> expr(new ArithExpression());
  expr->push_back_load_field(testHeader1, 0); // f32
  expr->push_back_load_const(Data(1));
  expr->push_back_op(ExprOpcode::ADD);
  expr->build();

  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_expression(std::move(expr));

  Field &f32 = phv->get_field(testHeader1, 0);
  f32.set(0xaba);

  Field &dst = phv->get_field(testHeader1, 3); // f16
  dst.set(0);

  ASSERT_EQ((unsigned) 0, dst.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ((unsigned) 0xabb, dst.get_uint());
}

TEST_F(ActionsTest, SetFromConstStress) {
  Data value(0xaba);
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv->get_field(testHeader1, 3); // f16

  for(int i = 0; i < 100000; i++) {
    f.set(0);
    ASSERT_EQ((unsigned) 0, f.get_uint());
    testActionFnEntry(pkt.get());
    ASSERT_EQ((unsigned) 0xaba, f.get_uint());
  }
}

TEST_F(ActionsTest, CopyHeader) {
  Header &hdr1 = phv->get_header(testHeader1);
  ASSERT_FALSE(hdr1.is_valid());

  Header &hdr2 = phv->get_header(testHeader2);
  ASSERT_FALSE(hdr2.is_valid());

  for(unsigned int i = 0; i < hdr1.size(); i++) {
    hdr1[i].set(0);
  }

  for(unsigned int i = 0; i < hdr2.size(); i++) {
    hdr2[i].set(i + 1);
  }

  CopyHeader primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_header(testHeader1);
  testActionFn.parameter_push_back_header(testHeader2);
  
  testActionFnEntry(pkt.get());
  ASSERT_FALSE(hdr1.is_valid());
  ASSERT_FALSE(hdr2.is_valid());
  for(unsigned int i = 0; i < hdr2.size(); i++) {
    ASSERT_EQ(0, hdr1[i]);
  }

  hdr2.mark_valid();
  testActionFnEntry(pkt.get());
  ASSERT_TRUE(hdr1.is_valid());
  for(unsigned int i = 0; i < hdr1.size(); i++) {
    ASSERT_EQ(i + 1, hdr1[i]);
  }
}

TEST_F(ActionsTest, CRSet) {
  CRSet primitive;
  testActionFn.push_back_primitive(&primitive);

  Field &f = phv->get_field(testHeader1, 3); // f16
  f.set(0);

  ASSERT_EQ((unsigned) 0, f.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ((unsigned) 666, f.get_uint());
}

TEST_F(ActionsTest, Pop) {
  Pop primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_header_stack(testHeaderStack);

  HeaderStack &stack = phv->get_header_stack(testHeaderStack);
  ASSERT_EQ(1u, stack.push_back());
  ASSERT_EQ(1u, stack.get_count());

  testActionFnEntry(pkt.get());
  ASSERT_EQ(0u, stack.get_count());
}

TEST_F(ActionsTest, ModifyFieldWithHashBasedOffset) {
  uint64_t base = 100;
  uint64_t size = 65536; // 16 bits

  BufBuilder builder;
  builder.push_back_field(testHeader1, 0);
  builder.push_back_field(testHeader1, 4);
  NamedCalculation calculation("test_calculation", 0, builder, "xxh64");

  ModifyFieldWithHashBasedOffset primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader2, 3); // f16
  testActionFn.parameter_push_back_const(Data(base));
  testActionFn.parameter_push_back_calculation(&calculation);
  testActionFn.parameter_push_back_const(Data(size));

  unsigned int expected = (base + calculation.output(*pkt)) % size;

  testActionFnEntry(pkt.get());
  ASSERT_EQ(expected, phv->get_field(testHeader2, 3).get_uint());
}

TEST_F(ActionsTest, ExecuteMeter) {
  typedef MeterArray::MeterErrorCode MeterErrorCode;
  typedef MeterArray::color_t color_t;
  MeterErrorCode rc;
  const color_t GREEN = 0;
  const color_t RED = 1;

  MeterArray meter_array("meter_test", 0, MeterArray::MeterType::PACKETS, 1, 64);
  // 10 packets per second, burst size of 5
  MeterArray::rate_config_t rate = {0.00001, 5};
  rc = meter_array.set_rates({rate});
  ASSERT_EQ(MeterErrorCode::SUCCESS, rc);

  ExecuteMeter primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 0); // f32
  testActionFn.parameter_push_back_meter_array(&meter_array);
  testActionFn.parameter_push_back_const(Data(16u)); // idx

  const Field &fdst = phv->get_field(testHeader1, 0);

  // send 5 packets very quickly, it is less than the burst size, so all packets
  // should be marked GREEN
  for(int i = 0; i < 5; i++) {
      testActionFnEntry(pkt.get());
      ASSERT_EQ(GREEN, fdst.get_uint());
  }
  // after the burst size, packets should be marked RED
  for(int i = 0; i < 5; i++) {
      testActionFnEntry(pkt.get());
      ASSERT_EQ(RED, fdst.get_uint());
  }
}

TEST_F(ActionsTest, TwoPrimitives) {
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_field(testHeader1, 0); // f32
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader2, 3); // f16
  testActionFn.parameter_push_back_field(testHeader2, 0); // f32

  Field &src1 = phv->get_field(testHeader1, 0); // 32
  Field &src2 = phv->get_field(testHeader2, 0); // 32
  src1.set(0xaba); src2.set(0xaba);

  Field &dst1 = phv->get_field(testHeader1, 3); // f16
  Field &dst2 = phv->get_field(testHeader2, 3); // f16
  dst1.set(0); dst2.set(0);

  ASSERT_EQ((unsigned) 0, dst1.get_uint());
  ASSERT_EQ((unsigned) 0, dst2.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ((unsigned) 0xaba, dst1.get_uint());
  ASSERT_EQ((unsigned) 0xaba, dst2.get_uint());
}
