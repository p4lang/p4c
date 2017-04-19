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

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/message_differencer.h>

#include <memory>
#include <fstream>
#include <streambuf>

using grpc::ClientContext;
using grpc::Status;

using google::protobuf::util::MessageDifferencer;

namespace {

const char *test_proto_txt = TESTDATADIR "/simple_router.proto.txt";

int get_table_id(const p4::config::P4Info &p4info,
                 const std::string &t_name) {
  for (const auto &table : p4info.tables()) {
    const auto &pre = table.preamble();
    if (pre.name() == t_name) return pre.id();
  }
  return 0;
}

int get_action_id(const p4::config::P4Info &p4info,
                  const std::string &a_name) {
  for (const auto &action : p4info.actions()) {
    const auto &pre = action.preamble();
    if (pre.name() == a_name) return pre.id();
  }
  return 0;
}

int get_mf_id(const p4::config::P4Info &p4info,
              const std::string &t_name, const std::string &mf_name) {
  for (const auto &table : p4info.tables()) {
    const auto &pre = table.preamble();
    if (pre.name() != t_name) continue;
    for (const auto &mf : table.match_fields())
      if (mf.name() == mf_name) return mf.id();
  }
  return -1;
}

int get_param_id(const p4::config::P4Info &p4info,
                 const std::string &a_name, const std::string &param_name) {
  for (const auto &action : p4info.actions()) {
    const auto &pre = action.preamble();
    if (pre.name() != a_name) continue;
    for (const auto &param : action.params())
      if (param.name() == param_name) return param.id();
  }
  return -1;
}

}  // namespace

int
main() {
  int dev_id = 0;

  auto channel = grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials());
  std::unique_ptr<p4::P4Runtime::Stub> pi_stub_(
      p4::P4Runtime::NewStub(channel));

  p4::config::P4Info p4info;
  std::ifstream istream(test_proto_txt);
  // p4info.ParseFromIstream(&istream);
  google::protobuf::io::IstreamInputStream istream_(&istream);
  google::protobuf::TextFormat::Parse(&istream_, &p4info);

  {
    p4::SetForwardingPipelineConfigRequest request;
    request.set_action(
        p4::SetForwardingPipelineConfigRequest_Action_VERIFY_AND_COMMIT);
    auto config = request.add_configs();
    config->set_device_id(dev_id);
    config->set_allocated_p4info(&p4info);

    p4::SetForwardingPipelineConfigResponse rep;
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

  p4::Entity entity;
  auto table_entry = entity.mutable_table_entry();
  table_entry->set_table_id(t_id);
  auto match = table_entry->add_match();
  match->set_field_id(mf_id);
  auto lpm = match->mutable_lpm();
  lpm->set_value(std::string("\x0a\x00\x00\x01", 4));  // 10.0.0.1
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
    p4::WriteRequest request;
    request.set_device_id(dev_id);
    auto update = request.add_updates();
    update->set_type(p4::Update_Type_INSERT);
    update->set_allocated_entity(&entity);
    ClientContext context;
    p4::WriteResponse rep;
    auto status = pi_stub_->Write(&context, request, &rep);
    assert(status.ok());
    update->release_entity();
  }

  auto read_one = [&dev_id, &pi_stub_, &table_entry] () {
    p4::ReadRequest request;
    request.set_device_id(dev_id);
    auto entity = request.add_entities();
    entity->set_allocated_table_entry(table_entry);
    ClientContext context;
    std::unique_ptr<grpc::ClientReader<p4::ReadResponse> > reader(
        pi_stub_->Read(&context, request));
    p4::ReadResponse rep;
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
    p4::WriteRequest request;
    request.set_device_id(dev_id);
    auto update = request.add_updates();
    update->set_type(p4::Update_Type_DELETE);
    update->set_allocated_entity(&entity);
    ClientContext context;
    p4::WriteResponse rep;
    auto status = pi_stub_->Write(&context, request, &rep);
    assert(status.ok());
    update->release_entity();
  }

  // check entry is indeed gone
  {
    auto rep = read_one();
    assert(rep.entities().size() == 0);
  }

  return 0;
}
