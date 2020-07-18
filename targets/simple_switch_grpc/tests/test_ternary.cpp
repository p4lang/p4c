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
#include <p4/v1/p4runtime.grpc.pb.h>

#include <google/protobuf/util/message_differencer.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "base_test.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

using google::protobuf::util::MessageDifferencer;

constexpr char ternary_json[] = TESTDATADIR "/ternary.json";
constexpr char ternary_proto[] = TESTDATADIR "/ternary.proto.txt";

class SimpleSwitchGrpcTest_Ternary : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_Ternary()
      : SimpleSwitchGrpcBaseTest(ternary_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(ternary_json);
    t_id = get_table_id(p4info, "ingress.ter");
    mf_id = get_mf_id(p4info, "ingress.ter", "h.hdr.f1");
    a_nop_id = get_action_id(p4info, "NoAction");
    a_s1_id = get_action_id(p4info, "ingress.send_1");
    a_s2_id = get_action_id(p4info, "ingress.send_2");
  }

  p4v1::Entity make_entry(const std::string &v, const std::string &mask,
                          int32_t priority, int a_id) const {
    p4v1::Entity entity;
    auto table_entry = entity.mutable_table_entry();
    table_entry->set_table_id(t_id);
    auto match = table_entry->add_match();
    match->set_field_id(mf_id);
    auto ternary = match->mutable_ternary();
    ternary->set_value(v);
    ternary->set_mask(mask);
    table_entry->set_priority(priority);
    auto table_action = table_entry->mutable_action();
    auto action = table_action->mutable_action();
    action->set_action_id(a_id);
    return entity;
  }

  grpc::Status add_entry(const p4v1::Entity &entry) const {
    return insert(entry);
  }

  grpc::Status read_entry(const p4v1::Entity &entry,
                          p4v1::ReadResponse *rep) const {
    return read(entry, rep);
  }

  grpc::Status send_and_receive(const std::string &pkt, int ig_port,
                                p4::bm::PacketStreamResponse *rcv_pkt) {
    static uint64_t id = 1;
    p4::bm::PacketStreamRequest request;
    request.set_id(id++);
    request.set_device_id(device_id);
    request.set_port(ig_port);
    request.set_packet(pkt);
    ClientContext context;
    auto stream = dataplane_stub->PacketStream(&context);
    stream->Write(request);
    stream->Read(rcv_pkt);
    stream->WritesDone();
    return stream->Finish();
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
  int t_id;
  int mf_id;
  int a_nop_id;
  int a_s1_id;
  int a_s2_id;
};

TEST_F(SimpleSwitchGrpcTest_Ternary, ReadEntry) {
  int priority = 128;
  auto entity = make_entry("\xaa\xbb", "\xff\xff", priority, a_s1_id);
  {
    auto status = add_entry(entity);
    EXPECT_TRUE(status.ok());
  }
  {
    p4v1::ReadResponse rep;
    auto status = read_entry(entity, &rep);
    EXPECT_TRUE(status.ok());
    ASSERT_EQ(1u, rep.entities().size());
    EXPECT_TRUE(MessageDifferencer::Equals(entity, rep.entities().Get(0)));
  }
}

TEST_F(SimpleSwitchGrpcTest_Ternary, PriorityOrder) {
  int priority_lo = 128, priority_hi = 333;
  int ig_port = 3;
  std::string pkt("\xaa\xbb");
  p4::bm::PacketStreamResponse rcv_pkt;
  auto entity_lo = make_entry("\xaa\xbb", "\xff\xff", priority_lo, a_s1_id);
  {
    auto status = add_entry(entity_lo);
    EXPECT_TRUE(status.ok());
  }
  {
    auto status = send_and_receive(pkt, ig_port, &rcv_pkt);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(1, rcv_pkt.port());
  }
  auto entity_hi = make_entry("\xaa\x0b", "\xff\x0f", priority_hi, a_s2_id);
  {
    auto status = add_entry(entity_hi);
    EXPECT_TRUE(status.ok());
  }
  {
    auto status = send_and_receive(pkt, ig_port, &rcv_pkt);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(2, rcv_pkt.port());
  }
}

// zero is not a valid priority value in P4Runtime
TEST_F(SimpleSwitchGrpcTest_Ternary, PriorityZero) {
  int priority = 0;
  std::string pkt("\xaa\xbb");
  p4::bm::PacketStreamResponse rcv_pkt;
  auto entity = make_entry("\xaa\xbb", "\xff\xff", priority, a_s1_id);
  auto status = add_entry(entity);
  EXPECT_FALSE(status.ok());
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
