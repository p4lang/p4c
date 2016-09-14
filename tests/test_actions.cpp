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
#include <chrono>
#include <thread>
#include <functional>

#include <cassert>

#include <bm/bm_sim/actions.h>

using namespace bm;

// some primitive definitions for testing

class SetField : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(SetField);

// more general Set operation, which can take any writable Data type
class Set : public ActionPrimitive<Data &, const Data &> {
  void operator ()(Data &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(Set);

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

// illustrates how a packet general purpose register can be used in a primitive
class WritePacketRegister : public ActionPrimitive<const Data &> {
  void operator ()(const Data &v) {
    get_packet().set_register(0, v.get<uint64_t>());
  }
};

REGISTER_PRIMITIVE(WritePacketRegister);

// Google Test fixture for actions tests
class ActionsTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  PHV *phv{nullptr};

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};
  header_id_t testHeaderS0{2}, testHeaderS1{3};

  header_stack_id_t testHeaderStack{0};

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  // pkt needs to be destroyed BEFORE phv_source, otherwise segfault
  // not a very clean design...
  std::unique_ptr<Packet> pkt{nullptr};

  ActionsTest()
      : testHeaderType("test_t", 0),
        testActionFn("test_action", 0),
        testActionFnEntry(&testActionFn),
        phv_source(PHVSourceIface::make_phv_source()) {
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
    phv_source->set_phv_factory(0, &phv_factory);
    pkt = std::unique_ptr<Packet>(new Packet(
        Packet::make_new(phv_source.get())));
    phv = pkt->get_phv();
  }

  virtual void TearDown() { }
};

// TODO(antonin)
// TEST_F(ActionsTest, NumParams) {
//   Data value(0xaba);
//   SetField primitive;
//   testActionFn.push_back_primitive(&primitive);
//   testActionFn.parameter_push_back_field(testHeader1, 3); // f16
//   testActionFn.parameter_push_back_const(value);

//   ASSERT_EQ(2u, testActionFn.num_params());
// }

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

TEST_F(ActionsTest, SetFromRegisterRef) {
  constexpr size_t register_size = 1024;
  constexpr int register_bw = 16;
  RegisterArray register_array("register_test", 0, register_size, register_bw);
  unsigned int value_i(0xaba);
  const unsigned int register_idx = 68;
  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_register_ref(&register_array, register_idx);

  Field &dst = phv->get_field(testHeader1, 3); // f16
  dst.set(0);

  register_array.at(register_idx).set(value_i);

  ASSERT_EQ(0u, dst.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ(value_i, dst.get_uint());
}

TEST_F(ActionsTest, SetFromRegisterGen) {
  constexpr size_t register_size = 1024;
  constexpr int register_bw = 16;
  RegisterArray register_array("register_test", 0, register_size, register_bw);

  // the register index is an expression
  std::unique_ptr<ArithExpression> expr_idx(new ArithExpression());
  expr_idx->push_back_load_field(testHeader1, 0); // f32
  expr_idx->push_back_load_const(Data(1));
  expr_idx->push_back_op(ExprOpcode::ADD);
  expr_idx->build();

  const unsigned int register_idx = 68;
  Field &f_idx = phv->get_field(testHeader1, 0); // f32
  f_idx.set(register_idx - 1);

  unsigned int value_i(0xaba);

  SetField primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_register_gen(&register_array,
                                                std::move(expr_idx));

  Field &dst = phv->get_field(testHeader1, 3); // f16
  dst.set(0);

  register_array.at(register_idx).set(value_i);

  ASSERT_EQ(0u, dst.get_uint());

  testActionFnEntry(pkt.get());

  ASSERT_EQ(value_i, dst.get_uint());
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

TEST_F(ActionsTest, Set) {
  unsigned int value_i(0xaba);
  Data value(value_i);
  Set primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_field(testHeader1, 3); // f16
  testActionFn.parameter_push_back_const(value);

  Field &f = phv->get_field(testHeader1, 3); // f16

  f.set(0);
  ASSERT_EQ(0u, f.get_uint());
  testActionFnEntry(pkt.get());
  ASSERT_EQ(value_i, f.get_uint());
}

TEST_F(ActionsTest, SetRegisterRef) {
  constexpr size_t register_size = 1024;
  constexpr int register_bw = 16;
  RegisterArray register_array("register_test", 0, register_size, register_bw);
  unsigned int value_i(0xaba);
  Data value(value_i);
  const unsigned int register_idx = 68;
  Set primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_register_ref(&register_array, register_idx);
  testActionFn.parameter_push_back_const(value);

  testActionFnEntry(pkt.get());
  ASSERT_EQ(value_i, register_array.at(register_idx).get_uint());
}

TEST_F(ActionsTest, SetRegisterGen) {
  constexpr size_t register_size = 1024;
  constexpr int register_bw = 16;
  RegisterArray register_array("register_test", 0, register_size, register_bw);

  // the register index is an expression
  std::unique_ptr<ArithExpression> expr_idx(new ArithExpression());
  expr_idx->push_back_load_field(testHeader1, 0); // f32
  expr_idx->push_back_load_const(Data(1));
  expr_idx->push_back_op(ExprOpcode::ADD);
  expr_idx->build();

  const unsigned int register_idx = 68;
  Field &f_idx = phv->get_field(testHeader1, 0); // f32
  f_idx.set(register_idx - 1);

  unsigned int value_i(0xaba);
  Data value(value_i);

  Set primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_register_gen(&register_array,
                                                std::move(expr_idx));
  testActionFn.parameter_push_back_const(value);

  testActionFnEntry(pkt.get());
  ASSERT_EQ(value_i, register_array.at(register_idx).get_uint());
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
    ASSERT_EQ(0u, hdr1[i].get_uint());
  }

  hdr2.mark_valid();
  testActionFnEntry(pkt.get());
  ASSERT_TRUE(hdr1.is_valid());
  for(unsigned int i = 0; i < hdr1.size(); i++) {
    if (hdr1[i].is_hidden()) break;
    ASSERT_EQ(i + 1, hdr1[i].get_uint());
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

TEST_F(ActionsTest, WritePacketRegister) {
  const uint64_t v = 12345u;
  const size_t idx = 0u;

  WritePacketRegister primitive;
  testActionFn.push_back_primitive(&primitive);
  testActionFn.parameter_push_back_const(Data(v));

  pkt->set_register(idx, 0u);
  ASSERT_EQ(0u, pkt->get_register(idx));

  testActionFnEntry(pkt.get());

  ASSERT_EQ(v, pkt->get_register(idx));
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

extern bool WITH_VALGRIND; // defined in main.cpp

// added this test after I found a race condition when the same action primitive
// is executed by 2 different threads
TEST_F(ActionsTest, ConcurrentPrimitiveExecution) {
  uint64_t base = 100;
  uint64_t size = 65536; // 16 bits

  BufBuilder builder;
  builder.push_back_field(testHeader1, 0);
  builder.push_back_field(testHeader1, 4);
  NamedCalculation calculation("test_calculation", 0, builder, "xxh64");

  auto primitive = ActionOpcodesMap::get_instance()->get_primitive(
      "ModifyFieldWithHashBasedOffset");

  testActionFn.push_back_primitive(primitive);
  testActionFn.parameter_push_back_field(testHeader2, 3); // f16
  testActionFn.parameter_push_back_const(Data(base));
  testActionFn.parameter_push_back_calculation(&calculation);
  testActionFn.parameter_push_back_const(Data(size));

  unsigned int expected = (base + calculation.output(*pkt)) % size;

  auto action_loop = [this, expected](size_t iters, ActionFnEntry &entry) {
    for (size_t i = 0; i < iters; i++) {
      auto pkt = std::unique_ptr<Packet>(new Packet(
          Packet::make_new(phv_source.get())));
      entry(pkt.get());
      ASSERT_EQ(expected, pkt->get_phv()->get_field(testHeader2, 3).get_uint());
    }
  };

  const size_t iterations = WITH_VALGRIND ? 50000u : 1000000u;

  std::thread t1(action_loop, iterations, std::ref(testActionFnEntry));
  std::thread t2(action_loop, iterations, std::ref(testActionFnEntry));
  t1.join(); t2.join();
}

// added this test after I found a deadlock when an action function accessing a
// register was executed by 2 different threads (the std::unique_lock was
// accessed by the 2 threads...)
// the register tests below were not enough to detect this because they use 2
// different action functions
TEST_F(ActionsTest, ConcurrentRegisterPrimitiveExecution) {
  constexpr size_t register_size = 256;
  constexpr int register_bw = 128;
  RegisterArray register_array("register_test", 0, register_size, register_bw);

  auto primitive = ActionOpcodesMap::get_instance()->get_primitive("Set");

  testActionFn.push_back_primitive(primitive);
  testActionFn.parameter_push_back_register_ref(&register_array, 12);
  testActionFn.parameter_push_back_const(Data(0xabababab));

  auto action_loop = [this](size_t iters, ActionFnEntry &entry) {
    for (size_t i = 0; i < iters; i++) {
      auto pkt = std::unique_ptr<Packet>(new Packet(
          Packet::make_new(phv_source.get())));
      entry(pkt.get());
    }
  };

  const size_t iterations = WITH_VALGRIND ? 50000u : 1000000u;

  std::thread t1(action_loop, iterations, std::ref(testActionFnEntry));
  std::thread t2(action_loop, iterations, std::ref(testActionFnEntry));
  t1.join(); t2.join();
}

class RegisterSpin : public ActionPrimitive<RegisterArray &, const Data &> {
  void operator ()(RegisterArray &register_array, const Data &ts) {
    register_array.at(0).set(ts);
    std::this_thread::sleep_for(std::chrono::milliseconds(ts.get_uint()));
  }
};

REGISTER_PRIMITIVE(RegisterSpin);

class ActionsTestRegisterProtection : public ActionsTest {
 protected:
  static constexpr unsigned int msecs_to_sleep = 1000u;

  static constexpr size_t register_size = 1024;
  static constexpr int register_bw = 16;

  RegisterArray register_array_1;

  RegisterSpin primitive_spin;

  ActionFn testActionFn_2;
  ActionFnEntry testActionFnEntry_2;

  ActionsTestRegisterProtection()
      : ActionsTest(),
        testActionFn_2("test_action_2", 1),
        testActionFnEntry_2(&testActionFn_2),
        register_array_1("register_test_1", 0, register_size, register_bw) {
    configure_one_action(&testActionFn, &register_array_1);
  }

  virtual void SetUp() {
    ActionsTest::SetUp();
  }

  // virtual void TearDown() { }

  void configure_one_action(ActionFn *action_fn,
                            RegisterArray *register_array) {
    action_fn->push_back_primitive(&primitive_spin);
    action_fn->parameter_push_back_register_array(register_array);
    action_fn->parameter_push_back_const(Data(msecs_to_sleep));
  }

  std::chrono::milliseconds::rep execute_both_actions() {
    using clock = std::chrono::system_clock;
    clock::time_point start = clock::now();

    std::thread t(&ActionFnEntry::operator(), &testActionFnEntry, pkt.get());
    testActionFnEntry_2(pkt.get());
    t.join();

    clock::time_point end = clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count();
  }
};

constexpr unsigned int ActionsTestRegisterProtection::msecs_to_sleep;

// both actions reference the same register so sequential execution
TEST_F(ActionsTestRegisterProtection, Sequential) {
  configure_one_action(&testActionFn_2, &register_array_1);

  auto timedelta = execute_both_actions();

  constexpr unsigned int expected_timedelta = msecs_to_sleep * 2;

  ASSERT_LT(expected_timedelta * 0.95, timedelta);
  ASSERT_GT(expected_timedelta * 1.2, timedelta);
}

// the actions referenced different registers so parallel execution
TEST_F(ActionsTestRegisterProtection, Parallel) {
  RegisterArray register_array_2("register_test_2", 1,
                                 register_size, register_bw);
  configure_one_action(&testActionFn_2, &register_array_2);

  auto timedelta = execute_both_actions();

  constexpr unsigned int expected_timedelta = msecs_to_sleep;

  ASSERT_LT(expected_timedelta * 0.95, timedelta);
  ASSERT_GT(expected_timedelta * 1.2, timedelta);
}

class SetAndSpin : public ActionPrimitive<Data &, const Data &, const Data &> {
  void operator ()(Data &dst, const Data &src, const Data &ts) {
    dst.set(src);
    std::this_thread::sleep_for(std::chrono::milliseconds(ts.get_uint()));
  }
};

REGISTER_PRIMITIVE(SetAndSpin);

TEST_F(ActionsTestRegisterProtection, SequentialAdvanced) {
  RegisterArray register_array_2("register_test_2", 1,
                                 register_size, register_bw);

  // this test is more tricky
  // we execute reg_2[reg_1[3] + 1] = 0xab and in parallel we access reg_1. The
  // actions need to be executed sequentially
  // we initialize reg_1[3] to 1, so at the end reg_2[2] must be equal to 0xab

  SetAndSpin primitive;

  // the register index is an expression
  std::unique_ptr<ArithExpression> expr_idx(new ArithExpression());
  expr_idx->push_back_load_const(Data(1));
  expr_idx->push_back_load_register_ref(&register_array_1, 3);
  expr_idx->push_back_op(ExprOpcode::ADD);
  expr_idx->build();

  testActionFn_2.push_back_primitive(&primitive);
  testActionFn_2.parameter_push_back_register_gen(&register_array_2,
                                                  std::move(expr_idx));
  testActionFn_2.parameter_push_back_const(Data(0xab));
  testActionFn_2.parameter_push_back_const(Data(msecs_to_sleep));

  register_array_1.at(0).set(0);
  register_array_1.at(3).set(1);
  register_array_2.at(2).set(0);

  using clock = std::chrono::system_clock;
  clock::time_point start = clock::now();

  std::thread t_1(&ActionFnEntry::operator(), &testActionFnEntry, pkt.get());
  std::thread t_2(&ActionFnEntry::operator(), &testActionFnEntry_2, pkt.get());
  {
    // just for the sake of an extra access
    auto lock = register_array_1.unique_lock();
    unsigned int v1 = register_array_1.at(0).get_uint();
    std::this_thread::sleep_for(std::chrono::milliseconds(msecs_to_sleep));
    unsigned int v2 = register_array_1.at(0).get_uint();
    ASSERT_EQ(v1, v2);
  }
  t_1.join();
  t_2.join();

  clock::time_point end = clock::now();

  auto timedelta = std::chrono::duration_cast<std::chrono::milliseconds>(
      end - start).count();

  ASSERT_EQ(0xabu, register_array_2.at(2).get_uint());

  constexpr unsigned int expected_timedelta = msecs_to_sleep * 3;

  ASSERT_LT(expected_timedelta * 0.95, timedelta);
  ASSERT_GT(expected_timedelta * 1.2, timedelta);
}
