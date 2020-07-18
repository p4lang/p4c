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

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "base_test.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

constexpr char meter_json[] = TESTDATADIR "/meter.json";
constexpr char meter_proto[] = TESTDATADIR "/meter.proto.txt";

class SimpleSwitchGrpcTest_Meter : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_Meter()
      : SimpleSwitchGrpcBaseTest(meter_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(meter_json);
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
};

TEST_F(SimpleSwitchGrpcTest_Meter, ReadDefault) {
  auto t_id = get_table_id(p4info, "ingress.t_redirect");
  auto mf_id = get_mf_id(p4info, "ingress.t_redirect", "sm.ingress_port");
  auto a_id = get_action_id(p4info, "ingress.port_redirect");

  p4v1::TableEntry table_entry;
  table_entry.set_table_id(t_id);
  auto match = table_entry.add_match();
  match->set_field_id(mf_id);
  auto exact = match->mutable_exact();
  exact->set_value("\x01\xff");
  auto table_action = table_entry.mutable_action();
  auto action = table_action->mutable_action();
  action->set_action_id(a_id);

  {
    p4v1::WriteRequest write_request;
    write_request.set_device_id(device_id);
    auto update = write_request.add_updates();
    update->set_type(p4v1::Update_Type_INSERT);
    auto entity = update->mutable_entity();
    entity->mutable_table_entry()->CopyFrom(table_entry);
    p4v1::WriteResponse write_response;
    ClientContext context;
    auto status = Write(&context, write_request, &write_response);
    EXPECT_TRUE(status.ok());
  }

  {
    p4v1::ReadRequest read_request;
    read_request.set_device_id(device_id);
    auto read_entity = read_request.add_entities();
    table_entry.mutable_meter_config();  // make sure meter config is read
    read_entity->mutable_table_entry()->CopyFrom(table_entry);

    ClientContext context;
    std::unique_ptr<grpc::ClientReader<p4v1::ReadResponse> > reader(
        p4runtime_stub->Read(&context, read_request));
    p4v1::ReadResponse read_response;
    reader->Read(&read_response);
    auto status = reader->Finish();
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(read_response.entities_size(), 1);
    const auto &read_entry = read_response.entities(0).table_entry();
    ASSERT_TRUE(read_entry.has_meter_config());
    const auto &meter_config = read_entry.meter_config();
    EXPECT_EQ(meter_config.pir(), -1);
    EXPECT_EQ(meter_config.pburst(), -1);
    EXPECT_EQ(meter_config.cir(), -1);
    EXPECT_EQ(meter_config.cburst(), -1);
  }
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
