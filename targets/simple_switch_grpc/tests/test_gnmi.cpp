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

#include <bm/bm_sim/logger.h>

#include <grpc++/grpc++.h>

#include <gnmi/gnmi.grpc.pb.h>
#include <p4/bm/dataplane_interface.grpc.pb.h>

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base_test.h"

namespace sswitch_grpc {

namespace testing {

namespace {

// we use this P4 program by default because the base class requires one, but
// any program would do: this test doesn't use the P4Runtime service.
constexpr char loopback_json[] = TESTDATADIR "/loopback.json";
constexpr char loopback_proto[] = TESTDATADIR "/loopback.proto.txt";

class GNMIPathBuilder {
 public:
  explicit GNMIPathBuilder(gnmi::Path *path)
      : path(path) { }

  GNMIPathBuilder &append(const std::string &name,
                          const std::map<std::string, std::string> &keys = {}) {
    auto *e = path->add_elem();
    e->set_name(name);
    e->mutable_key()->insert(keys.begin(), keys.end());
    return *this;
  }

 private:
  gnmi::Path *path;
};

// only supports basic paths without '...' or '*'
// works for all paths returned by a subscription
std::string gNMI_path_to_XPath(const gnmi::Path &prefix,
                               const gnmi::Path &path) {
  std::string xpath("/openconfig-interfaces:");
  auto process_path = [&xpath](const gnmi::Path &p) {
    for (const auto &elem : p.elem()) {
      xpath.append(elem.name());
      for (const auto &p : elem.key())
        xpath.append("[" + p.first + "='" + p.second + "']");
      xpath.append("/");
    }
  };
  process_path(prefix);
  process_path(path);
  xpath.pop_back();
  return xpath;
}

// Test fixture for testing the gNMI implementation for bmv2, which was done
// using sysrepo. Some of the logic being tested is in the target-independent
// P4Runtime server in p4lang/PI and we already have unit tests in that
// repo. These tests are meant specifically for the bmv2 config subscriber and
// operational state provider.
class SimpleSwitchGrpcTest_gNMI : public SimpleSwitchGrpcBaseTest {
 protected:
  using StreamType =
      grpc::ClientReaderWriter<gnmi::SubscribeRequest, gnmi::SubscribeResponse>;

  SimpleSwitchGrpcTest_gNMI()
      : SimpleSwitchGrpcBaseTest(loopback_proto),
        dataplane_channel(grpc::CreateChannel(
            dp_grpc_server_addr, grpc::InsecureChannelCredentials())),
        dataplane_stub(p4::bm::DataplaneInterface::NewStub(
            dataplane_channel)),
        gnmi_channel(grpc::CreateChannel(
            grpc_server_addr, grpc::InsecureChannelCredentials())),
        gnmi_stub(gnmi::gNMI::NewStub(gnmi_channel)) { }

  void SetUp() override {
    ASSERT_TRUE(cleanup_data_tree().ok());
  }

  void TearDown() override {
    clear_futures();
  }

  gnmi::SetRequest create_iface_req(const std::string &name,
                                    bool enabled = false) {
    gnmi::SetRequest req;
    GNMIPathBuilder pb(req.mutable_prefix());
    pb.append("interfaces").append("interface", {{"name", name}})
        .append("config");
    {
      auto *update = req.add_update();
      GNMIPathBuilder pb(update->mutable_path());
      pb.append("name");
      update->mutable_val()->set_string_val(name);
    }
    {
      auto *update = req.add_update();
      GNMIPathBuilder pb(update->mutable_path());
      pb.append("type");
      update->mutable_val()->set_string_val("iana-if-type:ethernetCsmacd");
    }
    {
      auto *update = req.add_update();
      GNMIPathBuilder pb(update->mutable_path());
      pb.append("enabled");
      update->mutable_val()->set_bool_val(enabled);
    }
    return req;
  }

  Status create_iface(const std::string &name, bool enabled = false) {
    auto req = create_iface_req(name, enabled);
    gnmi::SetResponse rep;
    ClientContext context;
    return gnmi_stub->Set(&context, req, &rep);
  }

  Status cleanup_data_tree() {
    gnmi::SetRequest req;
    GNMIPathBuilder pb(req.add_delete_());
    pb.append("interfaces").append("interface").append("...");
    gnmi::SetResponse rep;
    ClientContext context;
    return gnmi_stub->Set(&context, req, &rep);
  }

  Status set_port_oper_status(int port, p4::bm::PortOperStatus oper_status) {
    p4::bm::SetPortOperStatusRequest req;
    req.set_device_id(device_id);
    req.set_port(port);
    req.set_oper_status(oper_status);
    ClientContext context;
    google::protobuf::Empty rep;
    return dataplane_stub->SetPortOperStatus(&context, req, &rep);
  }

  // For the sake of simplicity we use a synchronous gRPC client which means we
  // cannot give a deadline for the Read call. We therefore wrap the Read call
  // in a std::future object and use std::future::wait_for() to specify a
  // timeout. When the timeout expires the Read call is not cancelled. However,
  // as soon as the client calls WritesDone on the stream, Read will return
  // false and the future will complete.
  std::future<bool> &ReadFuture(StreamType *stream,
                                gnmi::SubscribeResponse *rep) {
    futures.emplace_back(std::async(
        std::launch::async, [stream, rep]{ return stream->Read(rep); }));
    return futures.back();
  }

  // destroying the futures will not block if the client has called WritesDone
  void clear_futures() {
    futures.clear();
  }

  static std::string make_iface_name(int port, const std::string &name) {
    return std::to_string(device_id) + "-" + std::to_string(port) + "@" + name;
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
  std::shared_ptr<grpc::Channel> gnmi_channel{nullptr};
  std::unique_ptr<gnmi::gNMI::Stub> gnmi_stub{nullptr};

  using milliseconds = std::chrono::milliseconds;
  const milliseconds timeout{500};
  std::vector<std::future<bool> > futures;
};

// Change a port's operational status using the
// DataplaneInterface.SetPortOperStatus RPC and verifies that notifications are
// being generated correctly.
TEST_F(SimpleSwitchGrpcTest_gNMI, PortOperStatusUpdates) {
  const int port = 1;
  const std::string iface_name = make_iface_name(port, "eth1");
  EXPECT_TRUE(create_iface(iface_name, true).ok());
  EXPECT_TRUE(set_port_oper_status(port, p4::bm::OPER_STATUS_UP).ok());

  gnmi::SubscribeRequest req;
  gnmi::SubscribeResponse rep;
  auto *subList = req.mutable_subscribe();
  subList->set_mode(gnmi::SubscriptionList::STREAM);
  subList->set_updates_only(true);
  auto *sub = subList->add_subscription();
  GNMIPathBuilder pb(sub->mutable_path());
  pb.append("interfaces").append("interface").append("...");
  sub->set_mode(gnmi::ON_CHANGE);
  ClientContext context;
  auto stream = gnmi_stub->Subscribe(&context);
  EXPECT_TRUE(stream->Write(req));

  EXPECT_TRUE(stream->Read(&rep));
  EXPECT_TRUE(rep.sync_response());

  auto check_update = [&rep, &iface_name](const std::string &expected) {
    ASSERT_EQ(rep.response_case(), gnmi::SubscribeResponse::kUpdate);
    const auto &notification = rep.update();
    const auto &updates = notification.update();
    std::string expected_xpath("/openconfig-interfaces:interfaces/interface");
    expected_xpath.append("[name='").append(iface_name).append("']/")
        .append("state/oper-status");
    for (const auto &update : updates) {
      auto xpath = gNMI_path_to_XPath(notification.prefix(), update.path());
      if (xpath == expected_xpath) {
        EXPECT_EQ(update.val().string_val(), expected);
        return;
      }
    }
  };

  {
    EXPECT_TRUE(set_port_oper_status(port, p4::bm::OPER_STATUS_DOWN).ok());
    auto &f = ReadFuture(stream.get(), &rep);
    ASSERT_EQ(f.wait_for(timeout), std::future_status::ready);
    check_update("DOWN");
  }

  {
    EXPECT_TRUE(set_port_oper_status(port, p4::bm::OPER_STATUS_UP).ok());
    auto &f = ReadFuture(stream.get(), &rep);
    ASSERT_EQ(f.wait_for(timeout), std::future_status::ready);
    check_update("UP");
  }

  EXPECT_TRUE(stream->WritesDone());
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
