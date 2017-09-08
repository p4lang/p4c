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

#include <p4/p4runtime.grpc.pb.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "base_test.h"

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char loopback_json[] = TESTDATADIR "/packet_io.json";
constexpr char loopback_proto[] = TESTDATADIR "/packet_io.proto.txt";

class SimpleSwitchGrpcTest_PacketIO : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_PacketIO()
      : SimpleSwitchGrpcBaseTest(loopback_proto) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(loopback_json);
  }
};

TEST_F(SimpleSwitchGrpcTest_PacketIO, SendAndReceiveCPUPacket) {
  std::string payload(10, '\xab');
  p4::StreamMessageRequest request;

  ClientContext context;
  auto stream = p4runtime_stub->StreamChannel(&context);

  // create a new master stream
  auto arbitration = request.mutable_arbitration();
  arbitration->set_device_id(device_id);
  auto election_id = arbitration->mutable_election_id();
  election_id->set_high(1);
  election_id->set_low(0);
  stream->Write(request);

  auto packet_out = request.mutable_packet();
  packet_out->set_payload(payload);
  stream->Write(request);

  p4::StreamMessageResponse response;
  size_t packet_count = 0;
  while (stream->Read(&response)) {
    if (response.update_case() == p4::StreamMessageResponse::kPacket) {
      const auto &packet_in = response.packet();
      EXPECT_EQ(packet_in.payload(), payload);
      packet_count++;
      break;
    }
  }
  EXPECT_EQ(packet_count, 1);

  stream->WritesDone();
  auto status = stream->Finish();
  EXPECT_TRUE(status.ok());
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
