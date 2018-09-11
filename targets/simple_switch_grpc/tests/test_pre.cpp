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

#include <grpc++/grpc++.h>

#include <google/rpc/status.pb.h>
#include <p4/bm/dataplane_interface.grpc.pb.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base_test.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

using ::grpc::Status;

struct Replica {
  int32_t port;
  int32_t rid;

  Replica(int32_t port, int32_t rid)
      : port(port), rid(rid) { }

  bool operator <(const Replica &other) const {
    if (rid == other.rid) return port < other.port;
    return rid < other.rid;
  }

  bool operator ==(const Replica &other) const {
    return (rid == other.rid) && (port == other.port);
  }

  bool operator !=(const Replica &other) const {
    return !(*this == other);
  }
};

template <typename Entry,
          Entry *(::p4v1::PacketReplicationEngineEntry::*Accessor)()>
class SimpleSwitchGrpcTest_PreBase : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_PreBase(const char *json_path, const char *proto_path)
      : SimpleSwitchGrpcBaseTest(proto_path),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)),
        json_path(json_path) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(json_path);
  }

  Status create_entry(const Entry &entry) {
    return write_entry(entry, p4v1::Update::INSERT);
  }

  Status modify_entry(const Entry &entry) {
    return write_entry(entry, p4v1::Update::MODIFY);
  }

  Status delete_entry(const Entry &entry) {
    return write_entry(entry, p4v1::Update::DELETE);
  }

  struct ReplicaMgr {
    explicit ReplicaMgr(Entry *entry)
        : entry(entry) { }

    ReplicaMgr &push_back(const Replica &replica) {
      auto r = entry->add_replicas();
      r->set_egress_port(replica.port);
      r->set_instance(replica.rid);
      return *this;
    }

    void pop_back() {
      entry->mutable_replicas()->RemoveLast();
    }

    size_t size() const { return entry->replicas_size(); }

    std::set<Replica> as_set() const {
      std::set<Replica> s;
      for (const auto &r : entry->replicas())
        s.emplace(r.egress_port(), r.instance());
      return s;
    }

    Entry *entry;
  };

  using StreamType = grpc::ClientReaderWriter<p4::bm::PacketStreamRequest,
                                              p4::bm::PacketStreamResponse>;

  // For the sake of simplicity we use a synchronous gRPC client which means we
  // cannot give a deadline for the Read call. We therefore wrap the Read call
  // in a std::future object and use std::future::wait_for() to specify a
  // timeout. When the timeout expires the Read call is not cancelled. However,
  // as soon as the client calls WritesDone on the stream, Read will return
  // false and the future will complete.
  std::future<bool> &ReadFuture(StreamType *stream,
                                p4::bm::PacketStreamResponse *rep) {
    futures.emplace_back(std::async(
        std::launch::async, [stream, rep]{ return stream->Read(rep); }));
    return futures.back();
  }

  std::set<Replica> receive_packets(StreamType *stream, size_t count) {
    std::set<Replica> received;
    for (size_t i = 0; i < count; i++) {
      p4::bm::PacketStreamResponse rep;
      auto &f = ReadFuture(stream, &rep);
      if (f.wait_for(timeout) != std::future_status::ready) break;
      auto rid = static_cast<int32_t>(
          static_cast<unsigned char>(rep.packet()[0] << 8) +
          static_cast<unsigned char>(rep.packet()[1]));
      received.emplace(rep.port(), rid);
    }
    return received;
  }

  using milliseconds = std::chrono::milliseconds;
  const milliseconds timeout{500};
  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};

 private:
  Status write_entry(const Entry &entry, p4v1::Update::Type type) {
    p4v1::WriteRequest request;
    request.set_device_id(device_id);
    auto *update = request.add_updates();
    update->set_type(type);
    auto *entity = update->mutable_entity();
    auto *pre_entry = entity->mutable_packet_replication_engine_entry();
    (pre_entry->*Accessor)()->CopyFrom(entry);
    p4v1::WriteResponse response;
    ClientContext context;
    return Write(&context, request, &response);
  }

  const char *json_path;
  std::vector<std::future<bool> > futures;
};

// The multicast program parsers the first 16 bits of the packet and sets the
// multicast group metadata field to the parsed value. On egress, the first 16
// bits of the packet are replaced with the multicast rid.
constexpr char multicast_json[] = TESTDATADIR "/multicast.json";
constexpr char multicast_proto[] = TESTDATADIR "/multicast.proto.txt";

class SimpleSwitchGrpcTest_Multicast : public SimpleSwitchGrpcTest_PreBase<
  ::p4v1::MulticastGroupEntry,
  &::p4v1::PacketReplicationEngineEntry::mutable_multicast_group_entry> {
 protected:
  SimpleSwitchGrpcTest_Multicast()
      : SimpleSwitchGrpcTest_PreBase(multicast_json, multicast_proto) { }

  using GroupEntry = ::p4v1::MulticastGroupEntry;

  Status create_group(const GroupEntry &group) {
    return create_entry(group);
  }

  Status modify_group(const GroupEntry &group) {
    return modify_entry(group);
  }

  Status delete_group(const GroupEntry &group) {
    return delete_entry(group);
  }

  void send_packet(StreamType *stream, int16_t group_id, int32_t port = 1) {
    p4::bm::PacketStreamRequest packet_request;
    packet_request.set_device_id(device_id);
    packet_request.set_port(port);
    unsigned char payload[2];
    payload[0] = (group_id >> 8) & 0xff;
    payload[1] = group_id & 0xff;
    packet_request.set_packet(
        reinterpret_cast<char *>(payload), sizeof(payload));
    stream->Write(packet_request);
  }
};

TEST_F(SimpleSwitchGrpcTest_Multicast, 2Rids3Ports) {
  ClientContext context;
  auto stream = dataplane_stub->PacketStream(&context);

  int16_t group_id = 10;
  GroupEntry group;
  group.set_multicast_group_id(group_id);
  ReplicaMgr replicas(&group);

  // we use ASSERT_EQ instead of EXPECT_EQ to ensure that the test is killed in
  // case of failure; otherwise because of ReadFuture (our poor-man async read)
  // we would proceed through the test with outstanding Read calls which would
  // cause issues anyway.

  Replica r1(1, 1), r2(2, 2);
  replicas.push_back(r1).push_back(r2);
  {
    auto status = create_group(group);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), group_id);
    auto received = receive_packets(stream.get(), replicas.size());
    ASSERT_EQ(replicas.as_set(), received);
  }

  Replica r3(3, r1.rid);
  replicas.push_back(r3);
  {
    auto status = modify_group(group);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), group_id);
    auto received = receive_packets(stream.get(), replicas.size());
    ASSERT_EQ(replicas.as_set(), received);
  }

  replicas.pop_back();
  {
    auto status = modify_group(group);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), group_id);
    auto received = receive_packets(stream.get(), replicas.size());
    ASSERT_EQ(replicas.as_set(), received);
  }

  {
    auto status = delete_group(group);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), group_id);
    auto received = receive_packets(stream.get(), 1);
    ASSERT_TRUE(received.empty());
  }

  stream->WritesDone();
  stream->Finish();
}

// The multicast program parsers the first 16 bits of the packet and clones
// (i2e) the packet to the session id specified by the parsed value. On egress,
// the first 16 bits of the packet are replaced with the (multicast) rid. The
// original packet is "loopbacked" on the ingress port.
constexpr char clone_json[] = TESTDATADIR "/clone.json";
constexpr char clone_proto[] = TESTDATADIR "/clone.proto.txt";

class SimpleSwitchGrpcTest_Clone : public SimpleSwitchGrpcTest_PreBase<
  ::p4v1::CloneSessionEntry,
  &::p4v1::PacketReplicationEngineEntry::mutable_clone_session_entry> {
 protected:
  SimpleSwitchGrpcTest_Clone()
      : SimpleSwitchGrpcTest_PreBase(clone_json, clone_proto) { }

  using SessionEntry = ::p4v1::CloneSessionEntry;

  Status create_session(const SessionEntry &session) {
    return create_entry(session);
  }

  Status modify_session(const SessionEntry &session) {
    return modify_entry(session);
  }

  Status delete_session(const SessionEntry &session) {
    return delete_entry(session);
  }

  void send_packet(StreamType *stream, int16_t session_id, int32_t port = 1) {
    p4::bm::PacketStreamRequest packet_request;
    packet_request.set_device_id(device_id);
    packet_request.set_port(port);
    unsigned char payload[2];
    payload[0] = (session_id >> 8) & 0xff;
    payload[1] = session_id & 0xff;
    packet_request.set_packet(
        reinterpret_cast<char *>(payload), sizeof(payload));
    stream->Write(packet_request);
  }
};

TEST_F(SimpleSwitchGrpcTest_Clone, SingletonAndMulticast) {
  ClientContext context;
  auto stream = dataplane_stub->PacketStream(&context);

  int16_t session_id = 10;
  SessionEntry session;
  session.set_session_id(session_id);
  ReplicaMgr replicas(&session);

  int ig_port = 1;

  // we use ASSERT_EQ instead of EXPECT_EQ to ensure that the test is killed in
  // case of failure; otherwise because of ReadFuture (our poor-man async read)
  // we would proceed through the test with outstanding Read calls which would
  // cause issues anyway.

  Replica r1(2  /* port */, 1  /* rid */);
  replicas.push_back(r1);
  {
    auto status = create_session(session);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), session_id, ig_port);
    // + 1 for original packet
    auto received = receive_packets(stream.get(), replicas.size() + 1);
    auto expected = replicas.as_set();
    expected.emplace(ig_port, 0  /* rid */);
    ASSERT_EQ(expected, received);
  }

  Replica r2(3  /* port */, 2  /* rid */);
  replicas.push_back(r2);
  {
    auto status = modify_session(session);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), session_id);
    auto received = receive_packets(stream.get(), replicas.size() + 1);
    auto expected = replicas.as_set();
    expected.emplace(ig_port, 0  /* rid */);
    ASSERT_EQ(expected, received);
  }

  {
    auto status = delete_session(session);
    EXPECT_TRUE(status.ok());
    send_packet(stream.get(), session_id);
    auto received = receive_packets(stream.get(), 1);
    std::set<Replica> expected({Replica(ig_port, 0  /* rid */)});
    ASSERT_EQ(expected, received);
  }

  stream->WritesDone();
  stream->Finish();
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
