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

#include <google/protobuf/util/message_differencer.h>

#include <memory>

#include "utils.h"

namespace testing {

using grpc::ClientContext;
using grpc::ClientReaderWriter;
using grpc::Status;

using google::protobuf::util::MessageDifferencer;

namespace {

const char *test_proto_txt = TESTDATADIR "/packet_io.proto.txt";

}  // namespace

int
test() {
  int dev_id = 1;

  auto channel = grpc::CreateChannel(
      "localhost:50052", grpc::InsecureChannelCredentials());
  std::unique_ptr<p4::P4Runtime::Stub> pi_stub_(
      p4::P4Runtime::NewStub(channel));

  auto p4info = parse_p4info(test_proto_txt);

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

  std::string payload(10, '\xab');
  p4::StreamMessageRequest request;
  auto packet_out = request.mutable_packet();
  packet_out->set_payload(payload);

  ClientContext context;
  auto stream = pi_stub_->StreamChannel(&context);
  stream->Write(request);

  p4::StreamMessageResponse response;
  size_t packet_count = 0;
  while (stream->Read(&response)) {
    if (response.update_case() == p4::StreamMessageResponse::kPacket) {
      const auto &packet_in = response.packet();
      assert(packet_in.payload() == payload);
      packet_count++;
      break;
    }
  }
  assert(packet_count == 1);

  stream->WritesDone();
  auto status = stream->Finish();
  assert(status.ok());

  return 0;
}

}  // namespace testing

int
main() {
  return testing::test();
}
