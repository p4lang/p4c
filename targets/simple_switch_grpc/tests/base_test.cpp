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

#include <grpc++/grpc++.h>

#include <p4/tmp/p4config.grpc.pb.h>

#include <fstream>
#include <streambuf>

#include "base_test.h"

namespace sswitch_grpc {

namespace testing {

constexpr char SimpleSwitchGrpcBaseTest::grpc_server_addr[];
constexpr char SimpleSwitchGrpcBaseTest::dp_grpc_server_addr[];
constexpr int SimpleSwitchGrpcBaseTest::cpu_port;
constexpr int SimpleSwitchGrpcBaseTest::device_id;

SimpleSwitchGrpcBaseTest::SimpleSwitchGrpcBaseTest(
    const char *p4info_proto_txt_path)
    : p4runtime_channel(grpc::CreateChannel(
          grpc_server_addr, grpc::InsecureChannelCredentials())),
      p4runtime_stub(p4::P4Runtime::NewStub(p4runtime_channel)) {
  p4info = parse_p4info(p4info_proto_txt_path);
}

void
SimpleSwitchGrpcBaseTest::update_json(const char *json_path) {
  p4::SetForwardingPipelineConfigRequest request;
  request.set_action(
      p4::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
  auto config = request.add_configs();
  config->set_device_id(device_id);
  p4::tmp::P4DeviceConfig device_config;
  std::ifstream istream(json_path);
  device_config.mutable_device_data()->assign(
      (std::istreambuf_iterator<char>(istream)),
       std::istreambuf_iterator<char>());
  device_config.SerializeToString(config->mutable_p4_device_config());

  p4::SetForwardingPipelineConfigResponse rep;
  ClientContext context;
  config->set_allocated_p4info(&p4info);
  auto status = p4runtime_stub->SetForwardingPipelineConfig(
      &context, request, &rep);
  config->release_p4info();
  ASSERT_TRUE(status.ok());
}

}  // namespace testing

}  // namespace sswitch_grpc
