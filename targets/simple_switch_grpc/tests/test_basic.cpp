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

#include <p4/v1/p4runtime.grpc.pb.h>

#include <gtest/gtest.h>

#include <google/protobuf/util/message_differencer.h>

#include <memory>
#include <string>

#include "base_test.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

using google::protobuf::util::MessageDifferencer;

constexpr char simple_router_json[] = TESTDATADIR "/simple_router.json";
constexpr char simple_router_proto[] = TESTDATADIR "/simple_router.proto.txt";

class SimpleSwitchGrpcTest_Basic : public SimpleSwitchGrpcBaseTest {
 protected:
  SimpleSwitchGrpcTest_Basic()
      : SimpleSwitchGrpcBaseTest(simple_router_proto) { }

  void SetUp() override {
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(simple_router_json);
  }
};

TEST_F(SimpleSwitchGrpcTest_Basic, Entries) {
  auto t_id = get_table_id(p4info, "ipv4_lpm");
  auto mf_id = get_mf_id(p4info, "ipv4_lpm", "ipv4.dstAddr");
  auto a_id = get_action_id(p4info, "set_nhop");
  auto p0_id = get_param_id(p4info, "set_nhop", "nhop_ipv4");
  auto p1_id = get_param_id(p4info, "set_nhop", "port");

  p4v1::Entity entity;
  auto table_entry = entity.mutable_table_entry();
  table_entry->set_table_id(t_id);
  auto match = table_entry->add_match();
  match->set_field_id(mf_id);
  auto lpm = match->mutable_lpm();
  lpm->set_value(std::string("\x0a\x00\x00\x00", 4));  // 10.0.0.0/16
  lpm->set_prefix_len(16);
  auto table_action = table_entry->mutable_action();
  auto action = table_action->mutable_action();
  action->set_action_id(a_id);
  {
    auto param = action->add_params();
    param->set_param_id(p0_id);
    param->set_value(std::string("\x0a\x00\x00\x01", 4));  // 10.0.0.1
  }
  {
    auto param = action->add_params();
    param->set_param_id(p1_id);
    param->set_value(std::string("\x00\x09", 2));
  }

  // add entry
  {
    p4v1::WriteRequest request;
    request.set_device_id(device_id);
    auto update = request.add_updates();
    update->set_type(p4v1::Update_Type_INSERT);
    update->set_allocated_entity(&entity);
    ClientContext context;
    p4v1::WriteResponse rep;
    auto status = Write(&context, request, &rep);
    EXPECT_TRUE(status.ok());
    update->release_entity();
  }

  auto read_one = [this, &table_entry] () {
    p4v1::ReadRequest request;
    request.set_device_id(device_id);
    auto entity = request.add_entities();
    entity->set_allocated_table_entry(table_entry);
    ClientContext context;
    std::unique_ptr<grpc::ClientReader<p4v1::ReadResponse> > reader(
        p4runtime_stub->Read(&context, request));
    p4v1::ReadResponse rep;
    reader->Read(&rep);
    auto status = reader->Finish();
    EXPECT_TRUE(status.ok());
    entity->release_table_entry();
    return rep;
  };

  // get entry, check it is the one we added
  {
    auto rep = read_one();
    EXPECT_EQ(1u, rep.entities().size());
    EXPECT_TRUE(MessageDifferencer::Equals(entity, rep.entities().Get(0)));
  }

  // remove entry
  {
    p4v1::WriteRequest request;
    request.set_device_id(device_id);
    auto update = request.add_updates();
    update->set_type(p4v1::Update_Type_DELETE);
    update->set_allocated_entity(&entity);
    ClientContext context;
    p4v1::WriteResponse rep;
    auto status = Write(&context, request, &rep);
    EXPECT_TRUE(status.ok());
    update->release_entity();
  }

  // check entry is indeed gone
  {
    auto rep = read_one();
    EXPECT_EQ(0u, rep.entities().size());
  }
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
