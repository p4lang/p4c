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

#include <grpcpp/grpcpp.h>

#include <gnmi/gnmi.grpc.pb.h>
#include <p4/bm/dataplane_interface.grpc.pb.h>

#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base_test.h"
#include "utils.h"

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
    SimpleSwitchGrpcBaseTest::SetUp();
    update_json(loopback_json);
  }

  void TearDown() override { }

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
    p4::bm::SetPortOperStatusResponse rep;
    return dataplane_stub->SetPortOperStatus(&context, req, &rep);
  }

  static std::string make_iface_name(int port, const std::string &name) {
    return std::to_string(device_id) + "-" + std::to_string(port) + "@" + name;
  }

  const gnmi::Update *find_update(const gnmi::Notification &notification,
                                  const std::string &iface_name,
                                  const std::string &suffix) {
    const auto &updates = notification.update();
    std::string expected_xpath("/openconfig-interfaces:interfaces/interface");
    expected_xpath.append("[name='").append(iface_name).append("']/")
        .append(suffix);
    for (const auto &update : updates) {
      auto xpath = gNMI_path_to_XPath(notification.prefix(), update.path());
      if (xpath == expected_xpath) return &update;
    }
    return nullptr;
  }

  template <typename T>
  Status do_get(const std::string &iface_name,
                const std::initializer_list<T> &elements,
                gnmi::GetResponse *rep) {
    gnmi::GetRequest req;
    GNMIPathBuilder pb(req.add_path());
    pb.append("interfaces").append("interface", {{"name", iface_name}});
    for (const auto &e : elements) pb.append(e);
    req.set_type(gnmi::GetRequest::ALL);
    ClientContext context;
    return gnmi_stub->Get(&context, req, rep);
  }

  std::shared_ptr<grpc::Channel> dataplane_channel{nullptr};
  std::unique_ptr<p4::bm::DataplaneInterface::Stub> dataplane_stub{nullptr};
  std::shared_ptr<grpc::Channel> gnmi_channel{nullptr};
  std::unique_ptr<gnmi::gNMI::Stub> gnmi_stub{nullptr};

  using milliseconds = std::chrono::milliseconds;
  const milliseconds timeout{500};
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
  StreamReceiver<StreamType, gnmi::SubscribeResponse> stream_receiver(
      stream.get());

  auto read_from_stream = [&]() {
    auto msg = stream_receiver.get(
        [](const gnmi::SubscribeResponse &) { return true; }, timeout);
    if (msg == nullptr) return false;
    rep.CopyFrom(*msg);
    return true;
  };

  EXPECT_TRUE(stream->Write(req));

  EXPECT_TRUE(read_from_stream());
  EXPECT_TRUE(rep.sync_response());

  auto check_status_update = [&](const std::string &expected) {
    ASSERT_EQ(rep.response_case(), gnmi::SubscribeResponse::kUpdate);
    auto *update = find_update(rep.update(), iface_name, "state/oper-status");
    ASSERT_TRUE(update != nullptr);
    EXPECT_EQ(update->val().string_val(), expected);
  };

  auto check_last_change_update = [&]() {
    ASSERT_EQ(rep.response_case(), gnmi::SubscribeResponse::kUpdate);
    auto *update = find_update(rep.update(), iface_name, "state/last-change");
    ASSERT_TRUE(update != nullptr);
  };

  {
    EXPECT_TRUE(set_port_oper_status(port, p4::bm::OPER_STATUS_DOWN).ok());
    EXPECT_TRUE(read_from_stream());
    check_status_update("DOWN");
    check_last_change_update();
  }

  {
    EXPECT_TRUE(set_port_oper_status(port, p4::bm::OPER_STATUS_UP).ok());
    EXPECT_TRUE(read_from_stream());
    check_status_update("UP");
    check_last_change_update();
  }

  // check that carrier-transitions > 0
  {
    gnmi::GetResponse rep;
    EXPECT_TRUE(do_get(
        iface_name, {"state", "counters", "carrier-transitions"}, &rep).ok());
    ASSERT_EQ(rep.notification_size(), 1);
    auto *update = find_update(
        rep.notification(0), iface_name, "state/counters/carrier-transitions");
    ASSERT_TRUE(update != nullptr);
    EXPECT_GT(update->val().uint_val(), 0);
  }

  EXPECT_TRUE(stream->WritesDone());
  EXPECT_TRUE(stream->Finish().ok());
}

// Injects packets using the DataplaneInterface service and checks that the port
// counters are incremented correctly.
TEST_F(SimpleSwitchGrpcTest_gNMI, PortCounters) {
  const int port = 1;
  const std::string iface_name = make_iface_name(port, "eth1");
  EXPECT_TRUE(create_iface(iface_name, true).ok());
  EXPECT_TRUE(set_port_oper_status(port, p4::bm::OPER_STATUS_UP).ok());

  auto check_counter = [&](const std::string &name, uint64_t expected) {
    gnmi::GetResponse rep;
    // c_str() call necessary to ensure template deduction succeeds
    EXPECT_TRUE(do_get(
        iface_name, {"state", "counters", name.c_str()}, &rep).ok());
    ASSERT_EQ(rep.notification_size(), 1);
    std::string suffix("state/counters/");
    suffix.append(name);
    auto *update = find_update(rep.notification(0), iface_name, suffix);
    ASSERT_TRUE(update != nullptr);
    EXPECT_EQ(update->val().uint_val(), expected);
  };

  ClientContext context;
  auto stream = dataplane_stub->PacketStream(&context);

  check_counter("in-unicast-pkts", 0);
  check_counter("in-octets", 0);
  check_counter("out-unicast-pkts", 0);
  check_counter("out-octets", 0);

  p4::bm::PacketStreamRequest request;
  request.set_device_id(device_id);
  request.set_port(port);
  request.set_packet(std::string(10, '\xab'));
  stream->Write(request);
  p4::bm::PacketStreamResponse response;
  stream->Read(&response);

  check_counter("in-unicast-pkts", 1);
  check_counter("in-octets", 10);
  check_counter("out-unicast-pkts", 1);
  check_counter("out-octets", 10);

  EXPECT_TRUE(stream->WritesDone());
  EXPECT_TRUE(stream->Finish().ok());

  // TODO(antonin): test that stats are cleared when interface is deleted?
}

}  // namespace

}  // namespace testing

}  // namespace sswitch_grpc
