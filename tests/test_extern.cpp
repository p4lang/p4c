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

#include <bm/bm_sim/extern.h>
#include <bm/bm_sim/P4Objects.h>

#include <string>
#include <vector>
#include <cassert>

using namespace bm;

class ExternCounter : public ExternType {
 public:
  static constexpr unsigned int PACKETS = 0;
  static constexpr unsigned int BYTES = 1;

  BM_EXTERN_ATTRIBUTES {
    BM_EXTERN_ATTRIBUTE_ADD(init_count);
    BM_EXTERN_ATTRIBUTE_ADD(type);
  }

  void reset() {
    count = init_count_;
  }

  void init() override {
    init_count_ = init_count.get<size_t>();
    type_ = type.get<unsigned int>();
    reset();
  }

  void increment() {
    if (type_ == PACKETS) {
      count++;
    } else if (type_ == BYTES) {
      count += get_packet().get_ingress_length();
    } else {
      assert(0);
    }
  }

  void increment_by(const Data &d) {
    count += d.get<size_t>();
  }

  size_t get() const {
    return count;
  }

 private:
  // declared attributes
  Data init_count{0};
  Data type{PACKETS};

  // implementation members
  unsigned int type_{0};
  size_t init_count_{0};
  size_t count{0};
};

// do not put these inside an anonymous namespace or some compilers may complain
BM_REGISTER_EXTERN(ExternCounter);
BM_REGISTER_EXTERN_METHOD(ExternCounter, increment);
BM_REGISTER_EXTERN_METHOD(ExternCounter, increment_by, const Data &);
BM_REGISTER_EXTERN_METHOD(ExternCounter, reset);
BM_REGISTER_EXTERN_METHOD(ExternCounter, get);

BM_REGISTER_EXTERN_W_NAME(counter2, ExternCounter);
BM_REGISTER_EXTERN_W_NAME_METHOD(counter2, ExternCounter, increment);
BM_REGISTER_EXTERN_W_NAME_METHOD(counter2, ExternCounter,
                                 increment_by, const Data &);
BM_REGISTER_EXTERN_W_NAME_METHOD(counter2, ExternCounter, reset);
BM_REGISTER_EXTERN_W_NAME_METHOD(counter2, ExternCounter, get);

constexpr unsigned int ExternCounter::PACKETS;
constexpr unsigned int ExternCounter::BYTES;

// this demonstrates how p4objects can be used in an extern for more advanced
// use cases
class ExternP4Objects : public ExternType {
 public:
  BM_EXTERN_ATTRIBUTES {
    BM_EXTERN_ATTRIBUTE_ADD(register_name);
  }

  void init() override {
    register_array = get_p4objects().get_register_array(register_name);
  }

  size_t read(const Data &idx) const {
    const auto idx_v = idx.get<size_t>();
    return register_array->at(idx_v).get<size_t>();
  }

  // not exposed as an extern method, for testing
  RegisterArray *get_register_array() const {
    return register_array;
  }

 private:
  // declared attributes
  std::string register_name{};

  // implementation members
  RegisterArray *register_array{nullptr};
};

BM_REGISTER_EXTERN(ExternP4Objects);
BM_REGISTER_EXTERN_METHOD(ExternP4Objects, read, const Data &);

// this demonstrates how expressions can be used as extern attributes
// this extern type has 2 attributes: an expression and one integral value
// (var1); it has one additional integral value which is not an attribute but
// can be set by a method (var2). The execute method evaluates the expression,
// using var1 and var2 as locals.
class ExternExpression : public ExternType {
 public:
  BM_EXTERN_ATTRIBUTES {
    BM_EXTERN_ATTRIBUTE_ADD(expr);
    BM_EXTERN_ATTRIBUTE_ADD(var1);
  }

  void init() override {
    vars.push_back(var1);
    vars.emplace_back(0);
  }

  void set_var2(const Data &v) {
    vars.at(1) = v;
  }

  void execute(Data &dest) const {
    auto phv = get_packet().get_phv();
    // 'vars' passed as locals to evaluate expression
    expr.eval_arith(*phv, &dest, vars);
  }

 private:
  // declared attributes
  Expression expr{};
  Data var1{0};

  // implementation members
  std::vector<Data> vars{};
};

BM_REGISTER_EXTERN(ExternExpression);
BM_REGISTER_EXTERN_METHOD(ExternExpression, set_var2, const Data &);
BM_REGISTER_EXTERN_METHOD(ExternExpression, execute, Data &);

namespace {

// Google Test fixture for extern tests
class ExternTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;
  PHV *phv{nullptr};

  HeaderType testHeaderType;
  header_id_t testHeader1{0}, testHeader2{1};

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};

  // pkt needs to be destroyed BEFORE phv_source, otherwise segfault
  // not a very clean design...
  std::unique_ptr<Packet> pkt{nullptr};

  ExternTest()
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
  }

  static ActionPrimitive_ *get_extern_primitive(
      const std::string &extern_name, const std::string &method_name) {
    return ActionOpcodesMap::get_instance()->get_primitive(
        "_" + extern_name + "_" + method_name);
  }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    pkt = std::unique_ptr<Packet>(new Packet(
        Packet::make_new(phv_source.get())));
    phv = pkt->get_phv();
  }

  // virtual void TearDown() { }
};

TEST_F(ExternTest, ExternCounterExecute) {
  auto extern_instance = ExternFactoryMap::get_instance()->get_extern_instance(
      "ExternCounter");
  extern_instance->_register_attributes();
  extern_instance->init();
  auto counter_instance = dynamic_cast<ExternCounter *>(extern_instance.get());
  ASSERT_NE(nullptr, counter_instance);

  auto primitive = get_extern_primitive("ExternCounter", "increment_by");

  size_t increment_value = 44u;
  testActionFn.push_back_primitive(primitive);
  testActionFn.parameter_push_back_extern_instance(extern_instance.get());
  testActionFn.parameter_push_back_const(Data(increment_value));

  ASSERT_EQ(0u, counter_instance->get());
  testActionFnEntry(pkt.get());
  ASSERT_EQ(increment_value, counter_instance->get());

  counter_instance->reset();

  ASSERT_EQ(0u, counter_instance->get());
  testActionFnEntry(pkt.get());
  ASSERT_EQ(increment_value, counter_instance->get());
}

TEST_F(ExternTest, ExternCounterSetAttribute) {
  size_t init_value = 999u;

  auto extern_instance = ExternFactoryMap::get_instance()->get_extern_instance(
      "ExternCounter");
  extern_instance->_register_attributes();
  ASSERT_TRUE(extern_instance->_has_attribute("init_count"));
  ASSERT_FALSE(extern_instance->_has_attribute("bad_attribute_name"));
  extern_instance->_set_attribute<Data>("init_count", Data(init_value));
  extern_instance->init();
  auto counter_instance = dynamic_cast<ExternCounter *>(extern_instance.get());
  ASSERT_NE(nullptr, counter_instance);

  auto primitive = get_extern_primitive("ExternCounter", "increment_by");

  size_t increment_value = 44u;
  testActionFn.push_back_primitive(primitive);
  testActionFn.parameter_push_back_extern_instance(extern_instance.get());
  testActionFn.parameter_push_back_const(Data(increment_value));

  testActionFnEntry(pkt.get());
  ASSERT_EQ(increment_value + init_value, counter_instance->get());
}

TEST_F(ExternTest, NameAndId) {
  auto extern_instance = ExternFactoryMap::get_instance()->get_extern_instance(
      "ExternCounter");
  const std::string name = "my_extern_instance";
  const p4object_id_t id = 22;
  extern_instance->_set_name_and_id(name, id);
  extern_instance->_register_attributes();
  extern_instance->init();

  ASSERT_EQ(name, extern_instance->get_name());
  ASSERT_EQ(id, extern_instance->get_id());
}

TEST_F(ExternTest, ExternP4Objects) {
  auto extern_instance = ExternFactoryMap::get_instance()->get_extern_instance(
      "ExternP4Objects");
  auto typed_instance = dynamic_cast<ExternP4Objects *>(extern_instance.get());
  ASSERT_NE(nullptr, typed_instance);
  extern_instance->_register_attributes();
  ASSERT_TRUE(extern_instance->_has_attribute("register_name"));
  extern_instance->_set_attribute<std::string>("register_name", "my_register");

  // build a P4Objects instance with just one register array for testing
  std::stringstream is;
  is << "{\"register_arrays\":[{\"name\":\"my_register\",\"id\":0,"
     << "\"size\":1024,\"bitwidth\":32}]}";
  P4Objects objects;
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
  auto register_array = objects.get_register_array("my_register");
  ASSERT_NE(nullptr, register_array);

  extern_instance->_set_p4objects(&objects);
  extern_instance->init();
  ASSERT_EQ(register_array, typed_instance->get_register_array());
}

TEST_F(ExternTest, ExternExpression) {
  auto extern_instance = ExternFactoryMap::get_instance()->get_extern_instance(
      "ExternExpression");
  extern_instance->_register_attributes();
  Data var1(77), var2(55);
  extern_instance->_set_attribute<Data>("var1", var1);
  Expression expr;
  expr.push_back_load_local(0);
  expr.push_back_load_local(1);
  expr.push_back_op(ExprOpcode::ADD);
  expr.build();
  extern_instance->_set_attribute<Expression>("expr", expr);
  extern_instance->init();

  auto primitive_1 = get_extern_primitive("ExternExpression", "set_var2");
  auto primitive_2 = get_extern_primitive("ExternExpression", "execute");
  testActionFn.push_back_primitive(primitive_1);
  testActionFn.parameter_push_back_extern_instance(extern_instance.get());
  testActionFn.parameter_push_back_const(var2);
  testActionFn.push_back_primitive(primitive_2);
  testActionFn.parameter_push_back_extern_instance(extern_instance.get());
  testActionFn.parameter_push_back_field(testHeader1, 0);  // f32
  testActionFnEntry(pkt.get());

  const auto &dst = pkt->get_phv()->get_field(testHeader1, 0);
  const auto expected = var1.get<int>() + var2.get<int>();
  ASSERT_EQ(expected, dst.get<int>());
}

TEST_F(ExternTest, ExternCounterRename) {
  // check the objects are both retrievable
  auto check_name = [](const std::string &name) {
    auto extern_instance =
        ExternFactoryMap::get_instance()->get_extern_instance(name);
    extern_instance->_register_attributes();
    extern_instance->init();
    auto counter_instance =
        dynamic_cast<ExternCounter *>(extern_instance.get());
    ASSERT_NE(nullptr, counter_instance);

    auto primitive = get_extern_primitive(name, "increment_by");
    ASSERT_NE(nullptr, primitive);
  };

  check_name("ExternCounter");
  check_name("counter2");
}

}  // namespace
