/* Copyright 2018-present Barefoot Networks, Inc.
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

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

#include "base_test.h"
#include "utils.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char digest_json[] = TESTDATADIR "/digest.json";
constexpr char digest_proto[] = TESTDATADIR "/digest.proto.txt";

class SimpleSwitchGrpcTest_Digest : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_Digest()
      : SimpleSwitchGrpcBaseTest(digest_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(digest_json);
    digest_id = get_digest_id(p4info, "L2_digest");
    stream_receiver.reset(
        new StreamReceiver<ReaderWriter, p4v1::StreamMessageResponse>(
            stream.get()));
  }

  void TearDown() override {
    stream->WritesDone();
    auto status = stream->Finish();
    EXPECT_TRUE(status.ok());
  }

  template<typename Rep1, typename Period1, typename Rep2, typename Period2>
  grpc::Status config_digest(
      int32_t max_list_size,
      const std::chrono::duration<Rep1, Period1> &max_timeout,
      const std::chrono::duration<Rep2, Period2> &ack_timeout,
      p4v1::Update::Type type = p4v1::Update::INSERT) {
    auto max_timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        max_timeout).count();
    auto ack_timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        ack_timeout).count();
    p4v1::WriteRequest request;
    auto *update = request.add_updates();
    update->set_type(type);
    auto *digest_entry = update->mutable_entity()->mutable_digest_entry();
    digest_entry->set_digest_id(digest_id);
    auto *config = digest_entry->mutable_config();
    config->set_max_list_size(max_list_size);
    config->set_max_timeout_ns(max_timeout_ns);
    config->set_ack_timeout_ns(ack_timeout_ns);
    ClientContext context;
    p4v1::WriteResponse rep;
    return Write(&context, request, &rep);
  }

  bool ack_digest(const p4v1::DigestList &digest) {
    p4v1::StreamMessageRequest request;
    auto *ack = request.mutable_digest_ack();
    ack->set_digest_id(digest.digest_id());
    ack->set_list_id(digest.list_id());
    return stream->Write(request);
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
  std::unique_ptr<p4v1::DigestList> receive_digest(
      const std::chrono::duration<Rep, Period> &timeout) {
    auto msg = stream_receiver->get(
        [](const p4v1::StreamMessageResponse &response) {
          return (response.update_case() ==
                  p4v1::StreamMessageResponse::kDigest); },
        timeout);
    if (msg == nullptr) return nullptr;
    return std::unique_ptr<p4v1::DigestList>(
        new p4v1::DigestList(std::move(msg->digest())));
  }

  std::unique_ptr<p4v1::DigestList> receive_digest() {
    return receive_digest(defaultTimeout);
  }

  static constexpr std::chrono::milliseconds timeout0ms{0};
  static constexpr std::chrono::milliseconds timeout1000ms{1000};
  static constexpr std::chrono::milliseconds defaultTimeout{1000};

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
  int digest_id;
  std::unique_ptr<StreamReceiver<ReaderWriter, p4v1::StreamMessageResponse> >
  stream_receiver{nullptr};
};

/* static */ constexpr std::chrono::milliseconds
SimpleSwitchGrpcTest_Digest::timeout0ms;
/* static */ constexpr std::chrono::milliseconds
SimpleSwitchGrpcTest_Digest::timeout1000ms;
/* static */ constexpr std::chrono::milliseconds
SimpleSwitchGrpcTest_Digest::defaultTimeout;

TEST_F(SimpleSwitchGrpcTest_Digest, OneSample) {
  ASSERT_TRUE(config_digest(1, timeout0ms, timeout0ms).ok());
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  EXPECT_TRUE(send_and_receive(smac).ok());
  auto digest = receive_digest();
  EXPECT_TRUE(digest != nullptr);
}

TEST_F(SimpleSwitchGrpcTest_Digest, Ack) {
  ASSERT_TRUE(config_digest(1, timeout0ms, timeout1000ms).ok());
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  ASSERT_TRUE(send_and_receive(smac).ok());
  auto digest = receive_digest();
  ASSERT_TRUE(digest != nullptr);
  ASSERT_TRUE(send_and_receive(smac).ok());
  EXPECT_TRUE(receive_digest(std::chrono::milliseconds(200)) == nullptr);
  ASSERT_TRUE(ack_digest(*digest));
  ASSERT_TRUE(send_and_receive(smac).ok());
  EXPECT_TRUE(receive_digest() != nullptr);
}

TEST_F(SimpleSwitchGrpcTest_Digest, DeleteDigest) {
  const std::string smac("\x11\x22\x33\x44\x55\x66");
  ASSERT_TRUE(config_digest(1, timeout0ms, timeout0ms).ok());  // no caching
  EXPECT_TRUE(send_and_receive(smac).ok());
  EXPECT_TRUE(receive_digest() != nullptr);
  ASSERT_TRUE(
      config_digest(1, timeout0ms, timeout0ms, p4v1::Update::DELETE).ok());
  EXPECT_TRUE(send_and_receive(smac).ok());
  EXPECT_TRUE(receive_digest(std::chrono::milliseconds(200)) == nullptr);
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
