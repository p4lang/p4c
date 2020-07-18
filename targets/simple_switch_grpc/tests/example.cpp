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

#include <google/rpc/code.pb.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include <google/protobuf/util/message_differencer.h>

#include <fstream>
#include <memory>
#include <streambuf>
#include <string>

#include "utils.h"

namespace p4v1 = ::p4::v1;

namespace sswitch_grpc {

namespace testing {

namespace {

using grpc::ClientContext;
using grpc::Status;

using google::protobuf::util::MessageDifferencer;

constexpr char test_json[] = TESTDATADIR "/simple_router.json";
constexpr char test_proto_txt[] = TESTDATADIR "/simple_router.proto.txt";

int
test() {
  int dev_id = 99;

  auto channel = grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials());
  std::unique_ptr<p4v1::P4Runtime::Stub> pi_stub_(
      p4v1::P4Runtime::NewStub(channel));

  auto p4info = parse_p4info(test_proto_txt);

  auto set_election_id = [](p4v1::Uint128 *election_id) {
    election_id->set_high(0);
    election_id->set_low(1);
  };

  // initial handshake: open bidirectional stream and advertise election
  // id. This stream needs to stay open for the lifetime of the controller.
  ClientContext stream_context;
  auto stream = pi_stub_->StreamChannel(&stream_context);
  {
    p4v1::StreamMessageRequest request;
    auto arbitration = request.mutable_arbitration();
    arbitration->set_device_id(dev_id);
    set_election_id(arbitration->mutable_election_id());
    stream->Write(request);
    p4v1::StreamMessageResponse response;
    stream->Read(&response);
    assert(response.update_case() == p4v1::StreamMessageResponse::kArbitration);
    assert(response.arbitration().status().code() == ::google::rpc::Code::OK);
  }

  {
    p4v1::SetForwardingPipelineConfigRequest request;
    request.set_device_id(dev_id);
    request.set_action(
        p4v1::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
    set_election_id(request.mutable_election_id());
    auto config = request.mutable_config();
    config->set_allocated_p4info(&p4info);
    std::ifstream istream(test_json);
    config->mutable_p4_device_config()->assign(
        (std::istreambuf_iterator<char>(istream)),
         std::istreambuf_iterator<char>());

    p4v1::SetForwardingPipelineConfigResponse rep;
    ClientContext context;
    auto status = pi_stub_->SetForwardingPipelineConfig(
        &context, request, &rep);
    assert(status.ok());
    config->release_p4info();
  }

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
    set_election_id(request.mutable_election_id());
    request.set_device_id(dev_id);
    auto update = request.add_updates();
    update->set_type(p4v1::Update_Type_INSERT);
    update->set_allocated_entity(&entity);
    ClientContext context;
    p4v1::WriteResponse rep;
    auto status = pi_stub_->Write(&context, request, &rep);
    assert(status.ok());
    update->release_entity();
  }

  auto read_one = [&dev_id, &pi_stub_, &table_entry] () {
    p4v1::ReadRequest request;
    request.set_device_id(dev_id);
    auto entity = request.add_entities();
    entity->set_allocated_table_entry(table_entry);
    ClientContext context;
    std::unique_ptr<grpc::ClientReader<p4v1::ReadResponse> > reader(
        pi_stub_->Read(&context, request));
    p4v1::ReadResponse rep;
    reader->Read(&rep);
    auto status = reader->Finish();
    assert(status.ok());
    entity->release_table_entry();
    return rep;
  };

  // get entry, check it is the one we added
  {
    auto rep = read_one();
    assert(rep.entities().size() == 1);
    assert(MessageDifferencer::Equals(entity, rep.entities().Get(0)));
  }

  // remove entry
  {
    p4v1::WriteRequest request;
    set_election_id(request.mutable_election_id());
    request.set_device_id(dev_id);
    auto update = request.add_updates();
    update->set_type(p4v1::Update_Type_DELETE);
    update->set_allocated_entity(&entity);
    ClientContext context;
    p4v1::WriteResponse rep;
    auto status = pi_stub_->Write(&context, request, &rep);
    assert(status.ok());
    update->release_entity();
  }

  // check entry is indeed gone
  {
    auto rep = read_one();
    assert(rep.entities().size() == 0);
  }

  {
    stream->WritesDone();
    p4v1::StreamMessageResponse response;
    while (stream->Read(&response)) { }
    auto status = stream->Finish();
    assert(status.ok());
  }

  return 0;
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc

int
main() {
  return sswitch_grpc::testing::test();
}
