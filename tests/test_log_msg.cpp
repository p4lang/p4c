/* Copyright 2019 RT-RK Computer Based System
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

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>

#include <bm/bm_sim/actions.h>
#include <bm/bm_sim/core/primitives.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/P4Objects.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "jsoncpp/json.h"

using bm::PHVFactory;
using bm::ActionFn;
using bm::ActionFnEntry;
using bm::PHVSourceIface;
using bm::ActionPrimitive_;
using bm::ActionOpcodesMap;
using bm::ActionParam;
using bm::Data;
using bm::Packet;
using bm::P4Objects;
using bm::LookupStructureFactory;

namespace fs = boost::filesystem;

class LogMessageTest : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;
  P4Objects objects;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};
  std::unique_ptr<Packet> pkt{nullptr};

  LogMessageTest()
      : testActionFn("test_primitive", 0, 1),
        testActionFnEntry(&testActionFn),
        phv_source(PHVSourceIface::make_phv_source()) { }

  virtual void SetUp() {
    phv_source->set_phv_factory(0, &phv_factory);
    pkt = std::unique_ptr<Packet>(new Packet(
        Packet::make_new(phv_source.get())));
  }

  virtual void TearDown() { }
};

namespace {

struct CheckLogs {
  CheckLogs() {
    bm::Logger::set_logger_ostream(ss);
  }

  ~CheckLogs() {
    bm::Logger::unset_logger();
  }

  bool contains(const char *expected) const {
    auto actual = str();
    auto found = actual.find(expected);
    return found != std::string::npos;
  }

  std::string str() const {
    return ss.str();
  }

  std::stringstream ss;
};

}  // namespace

TEST_F(LogMessageTest, LogMessageNoErrorVector) {
  CheckLogs logs;
  std::unique_ptr<ActionPrimitive_> primitive;
  std::string primitive_name("log_msg");
  primitive = ActionOpcodesMap::get_instance()->get_primitive("log_msg");
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  testActionFn.parameter_push_back_string("x1 = {} x2 = {}");
  testActionFn.parameter_start_vector();
  testActionFn.parameter_push_back_const(Data("0x01"));
  testActionFn.parameter_push_back_const(Data("0x02"));
  testActionFn.parameter_end_vector();

  testActionFnEntry(pkt.get());

  EXPECT_TRUE(logs.contains("x1 = 1 x2 = 2"));
}

TEST_F(LogMessageTest, LogMessageNoErrorPlainString) {
  CheckLogs logs;
  std::unique_ptr<ActionPrimitive_> primitive;
  primitive = ActionOpcodesMap::get_instance()->get_primitive("log_msg");
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  testActionFn.parameter_push_back_string("Simple plain string log message.");
  // the log_msg action primitive always expect 2 arguments, so we need an empty
  // "parameters vector"
  testActionFn.parameter_start_vector();
  testActionFn.parameter_end_vector();

  testActionFnEntry(pkt.get());

  EXPECT_TRUE(logs.contains("Simple plain string log message."));
}

TEST_F(LogMessageTest, LogMessageError) {
  std::unique_ptr<ActionPrimitive_> primitive;
  primitive = ActionOpcodesMap::get_instance()->get_primitive("log_msg");
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  testActionFn.parameter_push_back_string("Plain string log message with {}.");
  testActionFn.parameter_start_vector();
  testActionFn.parameter_end_vector();

  // required when running the tests under Valgrind
  // avoid warning from Google test caused by running the test in a
  // multithreaded environment (P4Objects create threads)
  // see
  // https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#death-test-styles
  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  EXPECT_DEATH(testActionFnEntry(pkt.get()), "");
}

TEST_F(LogMessageTest, InitObjectsNoError) {
  fs::path json_path = fs::path(TESTDATADIR) / fs::path("logging.json");
  std::ifstream is(json_path.string());
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
}
