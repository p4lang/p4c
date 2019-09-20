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
using bm::ActionParamVectorFn;
using bm::P4Objects;
using bm::LookupStructureFactory;

namespace fs = boost::filesystem;

class LoggingMessage_Test : public ::testing::Test {
 protected:
  PHVFactory phv_factory;

  ActionFn testActionFn;
  ActionFnEntry testActionFnEntry;
  P4Objects objects;

  std::unique_ptr<PHVSourceIface> phv_source{nullptr};
  std::unique_ptr<Packet> pkt{nullptr};

  LoggingMessage_Test()
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

TEST_F(LoggingMessage_Test, LoggingMessageNoErrorVector) {
  bm::Logger::set_logger_console();
  std::unique_ptr<ActionPrimitive_> primitive;
  std::string primitive_name("log_msg");
  primitive = ActionOpcodesMap::get_instance()->get_primitive("log_msg");
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  testActionFn.parameter_push_back_string("x1 = {} x2 = {}");

  ActionParamVectorFn params_vector(&testActionFn, primitive_name);
  params_vector.parameter_push_back_const(Data("0x01"));
  params_vector.parameter_push_back_const(Data("0x02"));
  params_vector.parameter_push_back_param_vector();

  std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
  std::ostringstream strCout;
  std::cout.rdbuf(strCout.rdbuf());

  testActionFnEntry(pkt.get());

  std::string s = strCout.str();
  std::cout.rdbuf(oldCoutStreamBuf);

  std::string::size_type found = s.find("x1 = 1 x2 = 2");
  ASSERT_NE(found, std::string::npos);
}

TEST_F(LoggingMessage_Test, LoggingMessageNoErrorPlainString) {
  bm::Logger::set_logger_console();
  std::unique_ptr<ActionPrimitive_> primitive;
  primitive = ActionOpcodesMap::get_instance()->get_primitive("log_msg");
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  testActionFn.parameter_push_back_string("Simple plain string log message.");

  std::streambuf* oldCoutStreamBuf = std::cout.rdbuf();
  std::ostringstream strCout;
  std::cout.rdbuf(strCout.rdbuf());

  testActionFnEntry(pkt.get());

  std::string s = strCout.str();
  std::cout.rdbuf(oldCoutStreamBuf);

  std::string::size_type found = s.find("Simple plain string log message.");
  ASSERT_NE(found, std::string::npos);
}

TEST_F(LoggingMessage_Test, LoggingMessageError) {
  bm::Logger::set_logger_console();
  std::unique_ptr<ActionPrimitive_> primitive;
  primitive = ActionOpcodesMap::get_instance()->get_primitive("log_msg");
  ASSERT_NE(nullptr, primitive);

  testActionFn.push_back_primitive(primitive.get());
  testActionFn.parameter_push_back_string("Plain string log message with {}.");

  EXPECT_DEATH(testActionFnEntry(pkt.get()), "");
}

TEST_F(LoggingMessage_Test, InitObjectsNoError) {
  fs::path json_path = fs::path(TESTDATADIR) / fs::path("logging.json");
  std::ifstream is(json_path.string());
  LookupStructureFactory factory;
  ASSERT_EQ(0, objects.init_objects(&is, &factory));
}
