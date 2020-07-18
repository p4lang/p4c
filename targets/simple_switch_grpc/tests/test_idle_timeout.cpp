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

#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/grpcpp.h>

#include <p4/bm/dataplane_interface.grpc.pb.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "base_test.h"
#include "utils.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char digest_json[] = TESTDATADIR "/digest.json";
constexpr char digest_proto[] = TESTDATADIR "/digest.proto.txt";

class SimpleSwitchGrpcTest_IdleTimeout : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_IdleTimeout()
      : SimpleSwitchGrpcBaseTest(digest_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(digest_json);
    table_id = get_table_id(p4info, "ingress.smac");
    stream_receiver.reset(
        new StreamReceiver<ReaderWriter, p4v1::StreamMessageResponse>(
            stream.get()));
    {  // disable digest generation by setting the default action to NoAction
      p4v1::WriteRequest req;
      auto *update = req.add_updates();
      update->set_type(p4v1::Update::MODIFY);
      auto *entry = update->mutable_entity()->mutable_table_entry();
      entry->set_table_id(table_id);
      entry->set_is_default_action(true);
      auto *action = entry->mutable_action()->mutable_action();
      action->set_action_id(get_action_id(p4info, "NoAction"));
      ClientContext context;
      p4v1::WriteResponse rep;
      EXPECT_TRUE(Write(&context, req, &rep).ok());
    }
  }

  void TearDown() override {
    stream->WritesDone();
    auto status = stream->Finish();
    EXPECT_TRUE(status.ok());
  }

  template<typename Rep, typename Period>
  p4v1::Entity make_entry(
      const std::string &smac,
      const std::chrono::duration<Rep, Period> &idle_timeout = 0) const {
    p4v1::Entity entity;
    auto table_entry = entity.mutable_table_entry();
    table_entry->set_table_id(table_id);
    auto match = table_entry->add_match();
    match->set_field_id(get_mf_id(p4info, "ingress.smac", "h.ethernet.smac"));
    match->mutable_exact()->set_value(smac);
    auto *action = table_entry->mutable_action()->mutable_action();
    action->set_action_id(get_action_id(p4info, "NoAction"));
    table_entry->set_idle_timeout_ns(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            idle_timeout).count());
    return entity;
  }

  grpc::Status send_and_receive(
      const std::string &smac,
      const std::string &ethertype = "\xab\xab",
      const std::string &dmac = std::string(6, '\x01'),
      int ig_port = 1) {
    static uint64_t id = 1;
    std::string pkt = dmac + smac + ethertype;
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
    stream->WritesDone();
    return stream->Finish();
  }

  template<typename Rep, typename Period>
  std::unique_ptr<p4v1::IdleTimeoutNotification> receive_notification(
      const std::chrono::duration<Rep, Period> &timeout) {
    auto msg = stream_receiver->get(
        [](const p4v1::StreamMessageResponse &response) {
          return (response.update_case() ==
                  p4v1::StreamMessageResponse::kIdleTimeoutNotification); },
        timeout);
    if (msg == nullptr) return nullptr;
    return std::unique_ptr<p4v1::IdleTimeoutNotification>(
        new p4v1::IdleTimeoutNotification(
            std::move(msg->idle_timeout_notification())));
  }

  std::unique_ptr<p4v1::IdleTimeoutNotification> receive_notification() {
    return receive_notification(defaultTimeout);
  }

  static constexpr std::chrono::milliseconds defaultTimeout{1000};

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
  int table_id;
  std::unique_ptr<StreamReceiver<ReaderWriter, p4v1::StreamMessageResponse> >
  stream_receiver{nullptr};
};

/* static */ constexpr std::chrono::milliseconds
SimpleSwitchGrpcTest_IdleTimeout::defaultTimeout;

TEST_F(SimpleSwitchGrpcTest_IdleTimeout, EntryExpire) {
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  const std::chrono::milliseconds idle_timeout{2000};
  auto entry = make_entry(smac, idle_timeout);
  EXPECT_TRUE(insert(entry).ok());
  // By default table sweeping happens every second, so we could wait for as
  // long as 3 seconds for the notification, even though the TTL value is set to
  // 2 seconds.
  auto notification = receive_notification(
      idle_timeout + std::chrono::milliseconds(1500));
  ASSERT_TRUE(notification != nullptr);
  ASSERT_EQ(notification->table_entry_size(), 1);
  entry.mutable_table_entry()->clear_action();
  using google::protobuf::util::MessageDifferencer;
  EXPECT_TRUE(MessageDifferencer::Equals(
      notification->table_entry(0), entry.table_entry()));
}

TEST_F(SimpleSwitchGrpcTest_IdleTimeout, NotifyAgain) {
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  const std::chrono::milliseconds idle_timeout{2000};
  auto entry = make_entry(smac, idle_timeout);
  EXPECT_TRUE(insert(entry).ok());
  {
    auto notification = receive_notification(
        idle_timeout + std::chrono::milliseconds(1500));
    ASSERT_TRUE(notification != nullptr);
  }
  // TODO(antonin)
  // The current bmv2 implementation uses a sweeper thread to generate idle
  // timeout notifications. It never generates a notification for the same entry
  // in 2 successive sweeps, but if the entry has not been hit by the 3rd sweep,
  // a duplicate notification will be sent. This does not match the P4Runtime
  // specification, which states that the target must generate a notification
  // after one more TTL (and not a fixed sweep interval), so we will have to fix
  // this in the future.
  {
    auto notification = receive_notification(
        idle_timeout + std::chrono::milliseconds(1500));
    ASSERT_TRUE(notification != nullptr);
  }
}

TEST_F(SimpleSwitchGrpcTest_IdleTimeout, ReadEntry) {
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  const std::chrono::milliseconds idle_timeout{2000};
  auto entry = make_entry(smac, idle_timeout);
  EXPECT_TRUE(insert(entry).ok());
  const auto &written_table_entry = entry.table_entry();

  {
    p4v1::ReadResponse rep;
    EXPECT_TRUE(read(entry, &rep).ok());
    ASSERT_EQ(rep.entities_size(), 1);
    ASSERT_TRUE(rep.entities(0).has_table_entry());
    const auto &read_table_entry = rep.entities(0).table_entry();
    using google::protobuf::util::MessageDifferencer;
    EXPECT_TRUE(MessageDifferencer::Equals(
        read_table_entry, written_table_entry));
  }

  std::this_thread::sleep_for(idle_timeout / 2);

  {
    p4v1::ReadResponse rep;
    entry.mutable_table_entry()->mutable_time_since_last_hit();
    EXPECT_TRUE(read(entry, &rep).ok());
    ASSERT_EQ(rep.entities_size(), 1);
    ASSERT_TRUE(rep.entities(0).has_table_entry());
    const auto &read_table_entry = rep.entities(0).table_entry();
    EXPECT_GT(read_table_entry.time_since_last_hit().elapsed_ns(), 0);
    EXPECT_LT(read_table_entry.time_since_last_hit().elapsed_ns(),
              std::chrono::duration_cast<std::chrono::nanoseconds>(
                  idle_timeout).count());
  }
}

TEST_F(SimpleSwitchGrpcTest_IdleTimeout, ModifyTTL) {
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  const std::chrono::milliseconds idle_timeout_1{30000};  // 30 seconds
  const std::chrono::milliseconds idle_timeout_2{4000};  // 4 seconds
  auto entry = make_entry(smac, idle_timeout_1);
  EXPECT_TRUE(insert(entry).ok());
  {
    auto notification = receive_notification(std::chrono::milliseconds(2000));
    EXPECT_TRUE(notification == nullptr);
  }
  entry.mutable_table_entry()->set_idle_timeout_ns(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          idle_timeout_2).count());
  EXPECT_TRUE(modify(entry).ok());
  {
    // bmv2 resets the TTL of the entry (resets the "timer") when the max TTL
    // is modified. The P4Runtime spec doesn't mandate a specific behavior...
    auto notification = receive_notification(
        idle_timeout_2 + std::chrono::milliseconds(1500));
    EXPECT_TRUE(notification != nullptr);
  }
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
