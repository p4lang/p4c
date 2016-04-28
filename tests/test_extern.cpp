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

#include <cassert>

#include <bm/bm_sim/extern.h>

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

BM_REGISTER_EXTERN(ExternCounter);
BM_REGISTER_EXTERN_METHOD(ExternCounter, increment);
BM_REGISTER_EXTERN_METHOD(ExternCounter, increment_by, const Data &);
BM_REGISTER_EXTERN_METHOD(ExternCounter, reset);
BM_REGISTER_EXTERN_METHOD(ExternCounter, get);

constexpr unsigned int ExternCounter::PACKETS;
constexpr unsigned int ExternCounter::BYTES;

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
