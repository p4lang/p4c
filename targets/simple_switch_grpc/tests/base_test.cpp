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

#include <grpcpp/grpcpp.h>

#include <google/rpc/code.pb.h>

#include <fstream>
#include <streambuf>

#include "base_test.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

constexpr char SimpleSwitchGrpcBaseTest::grpc_server_addr[];
constexpr char SimpleSwitchGrpcBaseTest::dp_grpc_server_addr[];
constexpr int SimpleSwitchGrpcBaseTest::cpu_port;
constexpr uint64_t SimpleSwitchGrpcBaseTest::device_id;

SimpleSwitchGrpcBaseTest::SimpleSwitchGrpcBaseTest(
    const char *p4info_proto_txt_path)
    : p4runtime_channel(grpc::CreateChannel(
          grpc_server_addr, grpc::InsecureChannelCredentials())),
      p4runtime_stub(p4v1::P4Runtime::NewStub(p4runtime_channel)) {
  p4info = parse_p4info(p4info_proto_txt_path);
}

void
SimpleSwitchGrpcBaseTest::SetUp() {
  stream = p4runtime_stub->StreamChannel(&stream_context);
  p4v1::StreamMessageRequest request;
  auto arbitration = request.mutable_arbitration();
  arbitration->set_device_id(device_id);
  set_election_id(arbitration->mutable_election_id());
  stream->Write(request);
  p4v1::StreamMessageResponse response;
  stream->Read(&response);
  ASSERT_EQ(response.update_case(), p4v1::StreamMessageResponse::kArbitration);
  ASSERT_EQ(response.arbitration().status().code(), ::google::rpc::Code::OK);
}

void
SimpleSwitchGrpcBaseTest::TearDown() {
  stream->WritesDone();
  p4v1::StreamMessageResponse response;
  while (stream->Read(&response)) { }
  auto status = stream->Finish();
  EXPECT_TRUE(status.ok());
}

void
SimpleSwitchGrpcBaseTest::update_json(const char *json_path) {
  p4v1::SetForwardingPipelineConfigRequest request;
  request.set_device_id(device_id);
  request.set_action(
      p4v1::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
  set_election_id(request.mutable_election_id());
  auto config = request.mutable_config();
  std::ifstream istream(json_path);
  ASSERT_TRUE(istream.good());
  config->mutable_p4_device_config()->assign(
      (std::istreambuf_iterator<char>(istream)),
       std::istreambuf_iterator<char>());

  p4v1::SetForwardingPipelineConfigResponse rep;
  ClientContext context;
  config->set_allocated_p4info(&p4info);
  auto status = p4runtime_stub->SetForwardingPipelineConfig(
      &context, request, &rep);
  config->release_p4info();
  ASSERT_TRUE(status.ok());
}

void
SimpleSwitchGrpcBaseTest::set_election_id(p4v1::Uint128 *election_id) const {
  election_id->set_high(0);
  election_id->set_low(1);
}

grpc::Status
SimpleSwitchGrpcBaseTest::write(const p4v1::Entity &entity,
                                p4v1::Update::Type type) const {
  p4v1::WriteRequest request;
  request.set_device_id(device_id);
  auto update = request.add_updates();
  update->set_type(type);
  update->mutable_entity()->CopyFrom(entity);
  ClientContext context;
  p4v1::WriteResponse rep;
  return Write(&context, request, &rep);
}

grpc::Status
SimpleSwitchGrpcBaseTest::insert(const p4v1::Entity &entity) const {
  return write(entity, p4v1::Update::INSERT);
}

grpc::Status
SimpleSwitchGrpcBaseTest::modify(const p4v1::Entity &entity) const {
  return write(entity, p4v1::Update::MODIFY);
}

grpc::Status
SimpleSwitchGrpcBaseTest::remove(const p4v1::Entity &entity) const {
  return write(entity, p4v1::Update::DELETE);
}

grpc::Status
SimpleSwitchGrpcBaseTest::read(const p4v1::Entity &entity,
                               p4v1::ReadResponse *rep) const {
  p4v1::ReadRequest request;
  request.set_device_id(device_id);
  request.add_entities()->CopyFrom(entity);
  ClientContext context;
  std::unique_ptr<grpc::ClientReader<p4v1::ReadResponse> > reader(
      p4runtime_stub->Read(&context, request));
  reader->Read(rep);
  return reader->Finish();
}

grpc::Status
SimpleSwitchGrpcBaseTest::Write(ClientContext *context,
                                p4v1::WriteRequest &request,
                                p4v1::WriteResponse *response) const {
  request.set_device_id(device_id);
  set_election_id(request.mutable_election_id());
  return p4runtime_stub->Write(context, request, response);
}

}  // namespace testing

}  // namespace sswitch_grpc
