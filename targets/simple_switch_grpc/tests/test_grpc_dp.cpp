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

#include <p4/bm/dataplane_interface.grpc.pb.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "base_test.h"

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char loopback_json[] = TESTDATADIR "/loopback.json";
constexpr char loopback_proto[] = TESTDATADIR "/loopback.proto.txt";

class SimpleSwitchGrpcTest_GrpcDataplane : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_GrpcDataplane()
      : SimpleSwitchGrpcBaseTest(loopback_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(loopback_json);
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
};

TEST_F(SimpleSwitchGrpcTest_GrpcDataplane, SendAndReceive) {
  p4::bm::PacketStreamRequest request;
  request.set_id(42);
  request.set_device_id(device_id);
  request.set_port(9);
  request.set_packet(std::string(10, '\xab'));
  ClientContext context1;
  auto stream = dataplane_stub->PacketStream(&context1);
  stream->Write(request);
  p4::bm::PacketStreamResponse response;
  stream->Read(&response);
  EXPECT_EQ(response.id(), request.id());
  EXPECT_EQ(request.device_id(), response.device_id());
  EXPECT_EQ(request.port(), response.port());
  EXPECT_EQ(request.packet(), response.packet());
  stream->WritesDone();
  auto status = stream->Finish();
  EXPECT_TRUE(status.ok());

  ClientContext context2;
  stream = dataplane_stub->PacketStream(&context2);
  stream->Write(request);
  stream->WritesDone();
  stream->Read(&response);
  EXPECT_EQ(response.id(), request.id());
  EXPECT_EQ(request.device_id(), response.device_id());
  EXPECT_EQ(request.port(), response.port());
  EXPECT_EQ(request.packet(), response.packet());
  status = stream->Finish();
  EXPECT_TRUE(status.ok());
}

// This tests ensure that an error status is returned if we try to have multiple
// connections to the gRPC DataplaneInterface service
TEST_F(SimpleSwitchGrpcTest_GrpcDataplane, MultipleClients) {
  ClientContext context1;
  auto stream1 = dataplane_stub->PacketStream(&context1);
  ClientContext context2;
  auto stream2 = dataplane_stub->PacketStream(&context2);
  p4::bm::PacketStreamResponse response;
  while (stream2->Read(&response)) { }
  EXPECT_TRUE(stream1->WritesDone());
  EXPECT_TRUE(stream2->WritesDone());
  auto status1 = stream1->Finish();
  auto status2 = stream2->Finish();
  EXPECT_TRUE(status1.ok());
  EXPECT_EQ(status2.error_code(), grpc::StatusCode::RESOURCE_EXHAUSTED);
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
