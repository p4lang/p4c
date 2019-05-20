/* Copyright 2019-present Barefoot Networks, Inc.
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

#include <p4/bm/dataplane_interface.grpc.pb.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>

#include "base_test.h"
#include "utils.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char act_prof_json[] = TESTDATADIR "/action_profile.json";
constexpr char act_prof_proto[] = TESTDATADIR "/action_profile.proto.txt";

std::string port_to_bytes(int port) {
  std::string v;
  v.push_back(static_cast<char>((port >> 8) & 0xff));
  v.push_back(static_cast<char>(port & 0xff));
  return v;
}

class SimpleSwitchGrpcTest_ActionProfile : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_ActionProfile()
      : SimpleSwitchGrpcBaseTest(act_prof_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(act_prof_json);
    table_id = get_table_id(p4info, "IndirectWS");
    action_id = get_action_id(p4info, "send");
    {  // oneshot programming
      p4v1::WriteRequest req;
      auto *update = req.add_updates();
      update->set_type(p4v1::Update::INSERT);
      auto *entry = update->mutable_entity()->mutable_table_entry();
      entry->set_table_id(table_id);
      auto *mf = entry->add_match();
      mf->set_field_id(get_mf_id(p4info, "IndirectWS", "h.hdr.in_"));
      mf->mutable_exact()->set_value("\xab");
      auto *oneshot =
          entry->mutable_action()->mutable_action_profile_action_set();
      for (auto port : {port_1, port_2}) {
        auto *action_entry = oneshot->add_action_profile_actions();
        auto *action = action_entry->mutable_action();
        action->set_action_id(action_id);
        auto *param = action->add_params();
        param->set_param_id(get_param_id(p4info, "send", "eg_port"));
        param->set_value(port_to_bytes(port));
        action_entry->set_weight(1);
        action_entry->set_watch(port);
      }
      ClientContext context;
      p4v1::WriteResponse rep;
      EXPECT_TRUE(Write(&context, req, &rep).ok());
    }
  }

  grpc::Status send_and_receive(const std::string &entropy, int *eg_port) {
    static uint64_t id = 1;
    std::string pkt = "\xab" /* in_ */ + entropy;
    p4::bm::PacketStreamRequest request;
    request.set_id(id++);
    request.set_device_id(device_id);
    request.set_port(ig_port);
    request.set_packet(pkt);
    p4::bm::PacketStreamResponse response;
    ClientContext context;
    auto stream = dataplane_stub->PacketStream(&context);
    stream->Write(request);
    stream->Read(&response);
    *eg_port = response.port();
    stream->WritesDone();
    return stream->Finish();
  }

  Status set_port_oper_status(int port, p4::bm::PortOperStatus oper_status) {
    p4::bm::SetPortOperStatusRequest req;
    req.set_device_id(device_id);
    req.set_port(port);
    req.set_oper_status(oper_status);
    ClientContext context;
    p4::bm::SetPortOperStatusResponse rep;
    return dataplane_stub->SetPortOperStatus(&context, req, &rep);
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
  int table_id;
  int action_id;
  int port_1{1}, port_2{2};
  int ig_port{99};
};

TEST_F(SimpleSwitchGrpcTest_ActionProfile, WatchPort) {
  std::string entropy("\x01\x02\x03\x04");
  int eg_port_0, eg_port;
  EXPECT_TRUE(send_and_receive(entropy, &eg_port_0).ok());
  EXPECT_TRUE(send_and_receive(entropy, &eg_port).ok());
  EXPECT_EQ(eg_port, eg_port_0);
  EXPECT_TRUE(set_port_oper_status(eg_port_0, p4::bm::OPER_STATUS_DOWN).ok());
  // More than enough time for the member to be deactivated
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(send_and_receive(entropy, &eg_port).ok());
  EXPECT_NE(eg_port, eg_port_0);
  EXPECT_TRUE(set_port_oper_status(eg_port_0, p4::bm::OPER_STATUS_UP).ok());
  // More than enough time for the member to be reactivated
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_TRUE(send_and_receive(entropy, &eg_port).ok());
  EXPECT_EQ(eg_port, eg_port_0);
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
