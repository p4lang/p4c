// Copyright (c) 2017, Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <grpcpp/grpcpp.h>

#include <p4/bm/dataplane_interface.grpc.pb.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "base_test.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char counter_json[] = TESTDATADIR "/counter.json";
constexpr char counter_proto[] = TESTDATADIR "/counter.proto.txt";

class SimpleSwitchGrpcTest_Counter : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_Counter()
      : SimpleSwitchGrpcBaseTest(counter_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(counter_json);
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
};

TEST_F(SimpleSwitchGrpcTest_Counter, CounterHit) {
  auto t_id = get_table_id(p4info, "t_redirect");
  auto mf_id = get_mf_id(p4info, "t_redirect", "sm.packet_length");
  auto a_id = get_action_id(p4info, "port_redirect");

  auto make_table_entry = [&](const std::string &key_string,
                              p4v1::TableEntry *table_entry) {
    table_entry->set_table_id(t_id);
    auto match = table_entry->add_match();
    match->set_field_id(mf_id);
    auto exact = match->mutable_exact();
    exact->set_value(key_string);
    auto table_action = table_entry->mutable_action();
    auto action = table_action->mutable_action();
    action->set_action_id(a_id);
  };

  auto write_one_entry = [&](const std::string &key_string) {
    p4v1::WriteRequest write_request;
    write_request.set_device_id(device_id);
    auto update = write_request.add_updates();
    update->set_type(p4v1::Update_Type_INSERT);
    auto entity = update->mutable_entity();
    make_table_entry(key_string, entity->mutable_table_entry());

    p4v1::WriteResponse write_response;
    ClientContext context;
    auto status = Write(&context, write_request, &write_response);
    EXPECT_TRUE(status.ok());
  };
  write_one_entry(std::string("\x00\x00\x00\x0a", 4));
  write_one_entry(std::string("\x00\x00\x00\x05", 4));

  auto send_one_packet = [&](const std::string &payload) {
    p4::bm::PacketStreamRequest packet_request;
    packet_request.set_device_id(device_id);
    packet_request.set_port(1);
    packet_request.set_packet(payload);

    ClientContext context;
    auto stream = dataplane_stub->PacketStream(&context);
    stream->Write(packet_request);
    p4::bm::PacketStreamResponse packet_response;
    stream->Read(&packet_response);
    EXPECT_EQ(packet_request.device_id(), packet_response.device_id());
    EXPECT_EQ(packet_request.port(), packet_response.port());
    EXPECT_EQ(packet_request.packet(), packet_response.packet());
    stream->WritesDone();
    auto status = stream->Finish();
    EXPECT_TRUE(status.ok());
  };
  send_one_packet(std::string(10, '\xab'));
  send_one_packet(std::string(5, '\xab'));

  auto read_one_counter = [&](const std::string &key_string,
                              int packet_count, int byte_count) {
    p4v1::ReadRequest read_request;
    read_request.set_device_id(device_id);
    auto read_entity = read_request.add_entities();
    auto direct_counter_entry = read_entity->mutable_direct_counter_entry();
    make_table_entry(key_string, direct_counter_entry->mutable_table_entry());

    ClientContext context;
    std::unique_ptr<grpc::ClientReader<p4v1::ReadResponse> > reader(
        p4runtime_stub->Read(&context, read_request));
    p4v1::ReadResponse read_response;
    reader->Read(&read_response);
    auto status = reader->Finish();
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(read_response.entities_size(), 1);
    EXPECT_TRUE(read_response.entities(0).has_direct_counter_entry());
    auto &data = read_response.entities(0).direct_counter_entry().data();
    EXPECT_EQ(data.packet_count(), packet_count);
    EXPECT_EQ(data.byte_count(), byte_count);
  };
  read_one_counter(std::string("\x00\x00\x00\x0a", 4), 1, 10);
  read_one_counter(std::string("\x00\x00\x00\x05", 4), 1, 5);

  auto read_one_counter_from_table_entry = [&](
      const std::string &key_string, int packet_count, int byte_count) {
    p4v1::ReadRequest read_request;
    read_request.set_device_id(device_id);
    auto read_entity = read_request.add_entities();
    auto table_entry = read_entity->mutable_table_entry();
    make_table_entry(key_string, table_entry);
    table_entry->mutable_counter_data();  // makes sure that counter is read

    ClientContext context;
    std::unique_ptr<grpc::ClientReader<p4v1::ReadResponse> > reader(
        p4runtime_stub->Read(&context, read_request));
    p4v1::ReadResponse read_response;
    reader->Read(&read_response);
    auto status = reader->Finish();
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(read_response.entities_size(), 1);
    EXPECT_TRUE(read_response.entities(0).has_table_entry());
    auto &counter_data = read_response.entities(0).table_entry().counter_data();
    EXPECT_EQ(counter_data.packet_count(), packet_count);
    EXPECT_EQ(counter_data.byte_count(), byte_count);
  };
  read_one_counter_from_table_entry(std::string("\x00\x00\x00\x0a", 4), 1, 10);
  read_one_counter_from_table_entry(std::string("\x00\x00\x00\x05", 4), 1, 5);
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
