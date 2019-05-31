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

#include <bm/bm_sim/options_parse.h>

#include <gtest/gtest.h>

#include <vector>

#include "base_test.h"
#include "switch_runner.h"

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char start_json[] = TESTDATADIR "/loopback.json";

class SimpleSwitchGrpcEnv : public ::testing::Environment {
 public:
  // We make the switch a shared resource for all tests. This is mainly because
  // simple_switch detaches threads.
  // TODO(antonin): the issue with this is that tests may affect each other; in
  // particular tests which modify port operational status.
  void SetUp() override {
    auto &runner = SimpleSwitchGrpcRunner::get_instance(
        true, SimpleSwitchGrpcBaseTest::grpc_server_addr,
        SimpleSwitchGrpcBaseTest::cpu_port,
        SimpleSwitchGrpcBaseTest::dp_grpc_server_addr);
    bm::OptionsParser parser;
    std::vector<const char *> argv = {"test", "--device-id", "3"};
#ifdef WITH_THRIFT
    argv.push_back("--thrift-port");
    argv.push_back("45459");
#endif  // WITH_THRIFT
    // you can uncomment this when debugging
    argv.push_back("--log-console");
    argv.push_back(start_json);
    auto argc = static_cast<int>(argv.size());
    parser.parse(argc, const_cast<char **>(argv.data()), nullptr);
    ASSERT_EQ(0, runner.init_and_start(parser));
  }

  void TearDown() override {
    SimpleSwitchGrpcRunner::get_instance().shutdown();
  }
};

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(
       new sswitch_grpc::testing::SimpleSwitchGrpcEnv);
  return RUN_ALL_TESTS();
}
