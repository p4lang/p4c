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

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/logger.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/transport.h>

#include <bm/PI/pi.h>

#include <PI/frontends/proto/logging.h>

#include <PI/proto/pi_server.h>

#include <PI/p4info/digests.h>
#include <PI/pi.h>
#include <PI/target/pi_imp.h>
#include <PI/target/pi_learn_imp.h>

#include <grpcpp/grpcpp.h>

#include <p4/bm/dataplane_interface.grpc.pb.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "simple_switch.h"

#include "switch_runner.h"

#ifdef WITH_SYSREPO
#include "switch_sysrepo.h"
#endif  // WITH_SYSREPO

#ifdef WITH_THRIFT
#include <bm/SimpleSwitch.h>
#include <bm/bm_runtime/bm_runtime.h>

namespace sswitch_runtime {
    shared_ptr<SimpleSwitchIf> get_handler(SimpleSwitch *sw);
}  // namespace sswitch_runtime
#endif  // WITH_THRIFT

namespace sswitch_grpc {

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using ServerReaderWriter = grpc::ServerReaderWriter<
  p4::bm::PacketStreamResponse, p4::bm::PacketStreamRequest>;

class DataplaneInterfaceServiceImpl
    : public p4::bm::DataplaneInterface::Service,
      public bm::DevMgrIface {
 public:
  explicit DataplaneInterfaceServiceImpl(bm::device_id_t device_id)
      : device_id(device_id) {
    p_monitor = bm::PortMonitorIface::make_passive(device_id);
  }

  void my_transmit_fn(port_t port_num, packet_id_t pkt_id, const char *buffer,
                      int len) {
    p4::bm::PacketStreamResponse response;
    response.set_device_id(device_id);
    response.set_port(port_num);
    response.set_packet(buffer, len);
    Lock lock(mutex);
    if (packet_id_translation.find(pkt_id) != packet_id_translation.end()) {
      response.set_id(packet_id_translation[pkt_id]);
    }
    if (active) {
      stream->Write(response);
      auto &stats = ports_stats[port_num];
      stats.out_packets += 1;
      stats.out_octets += len;
    }
  }

  bool get_packet_stream_status() {
    Lock lock(mutex);
    return active;
  }

 private:
  using Lock = std::lock_guard<std::mutex>;

  Status PacketStream(ServerContext *context,
                      ServerReaderWriter *stream) override {
    {
      Lock lock(mutex);
      if (!started)
        return Status(StatusCode::UNAVAILABLE, "not ready");
      if (active) {
        return Status(StatusCode::RESOURCE_EXHAUSTED,
                      "only one client authorized at a time");
      }
      active = true;
      this->context = context;
      this->stream = stream;
    }
    p4::bm::PacketStreamRequest request;
    while (this->stream->Read(&request)) {
      if (request.device_id() != device_id) {
        continue;
      }
      const auto &packet = request.packet();
      if (packet.empty()) continue;
      if (!pkt_handler) continue;
      {
        Lock lock(mutex);
        add_port_if_new(request.port());
        pkt_handler(request.port(), packet.data(), packet.size(), pkt_cookie);
        // Get the packet id of newly created packet and save it in
        // packet_id_translation map. The map will be used to populate the id
        // field of the transmitted packet.
        if (request.id() != 0) {
          // grpc service has a single thread. get_packet_id() will return the
          // packet id of the newly received packet.
          packet_id_translation[SimpleSwitch::get_packet_id()] = request.id();
        }
        // PortStats is a POD struct; it will be value-initialized to 0s if the
        // port key is not found in the map.
        auto &stats = ports_stats[request.port()];
        stats.in_packets += 1;
        stats.in_octets += packet.size();
      }
    }
    auto &runner = sswitch_grpc::SimpleSwitchGrpcRunner::get_instance();
    runner.block_until_all_packets_processed();
    Lock lock(mutex);
    active = false;
    return Status::OK;
  }

  Status SetPortOperStatus(
      ServerContext *context,
      const p4::bm::SetPortOperStatusRequest *request,
      p4::bm::SetPortOperStatusResponse *response) override {
    (void) context;
    (void) response;
    using PortStatus = bm::DevMgrIface::PortStatus;
    Lock lock(mutex);
    if (request->device_id() != device_id)
      return Status(StatusCode::INVALID_ARGUMENT, "Invalid device id");
    if (request->oper_status() ==
        p4::bm::PortOperStatus::OPER_STATUS_DOWN) {
      add_port_if_new(request->port());
      ports_oper_status[request->port()] = false;
      p_monitor->notify(request->port(), PortStatus::PORT_DOWN);
    } else if (request->oper_status() ==
               p4::bm::PortOperStatus::OPER_STATUS_UP) {
      add_port_if_new(request->port());
      ports_oper_status[request->port()] = true;
      p_monitor->notify(request->port(), PortStatus::PORT_UP);
    } else {
      return Status(StatusCode::INVALID_ARGUMENT, "Invalid oper status");
    }
    return Status::OK;
  }

  // Needs to be called with exclusive lock.
  // This is required to ensure that notifications get sent to the PortMonitor
  // in a valid order.
  void add_port_if_new(port_t port_num) {
    using PortStatus = bm::DevMgrIface::PortStatus;
    auto it = ports_oper_status.find(port_num);
    if (it != ports_oper_status.end()) return;
    p_monitor->notify(port_num, PortStatus::PORT_ADDED);
    p_monitor->notify(port_num, PortStatus::PORT_UP);
    ports_oper_status[port_num] = true;
  }

  ReturnCode port_add_(const std::string &iface_name, port_t port_num,
                       const PortExtras &port_extras) override {
    _BM_UNUSED(iface_name);
    _BM_UNUSED(port_num);
    _BM_UNUSED(port_extras);
    return ReturnCode::SUCCESS;
  }

  ReturnCode port_remove_(port_t port_num) override {
    _BM_UNUSED(port_num);
    return ReturnCode::SUCCESS;
  }

  void transmit_fn_(port_t port_num, const char *buffer, int len) override {
    my_transmit_fn(port_num, -1, buffer, len);
  }

  void start_() override {
    Lock lock(mutex);
    started = true;
  }

  ReturnCode set_packet_handler_(const PacketHandler &handler, void *cookie)
      override {
    pkt_handler = handler;
    pkt_cookie = cookie;
    return ReturnCode::SUCCESS;
  }

  bool port_is_up_(port_t port) const override {
    Lock lock(mutex);
    auto status_it = ports_oper_status.find(port);
    return (status_it == ports_oper_status.end()) ? true : status_it->second;
  }

  std::map<port_t, PortInfo> get_port_info_() const override {
    return {};
  }

  PortStats get_port_stats_(port_t port) const override {
    Lock lock(mutex);
    // does not compile since method is "const"
    // return ports_stats[port];
    auto it = ports_stats.find(port);
    return (it == ports_stats.end()) ? PortStats() : it->second;
  }

  PortStats clear_port_stats_(port_t port) override {
    PortStats stats{};  // value-initialized to 0
    Lock lock(mutex);
    // does not compile since method is "const"
    // return ports_stats[port];
    auto it = ports_stats.find(port);
    if (it != ports_stats.end()) {
      stats = it->second;
      it->second = {};
    }
    return stats;
  }

  bm::device_id_t device_id;
  // protects the shared state (active) and prevents concurrent Write calls by
  // different threads
  mutable std::mutex mutex{};
  bool started{false};
  bool active{false};
  ServerContext *context{nullptr};
  ServerReaderWriter *stream{nullptr};
  PacketHandler pkt_handler{};
  void *pkt_cookie{nullptr};
  std::unordered_map<port_t, bool> ports_oper_status{};
  std::unordered_map<port_t, PortStats> ports_stats{};
  std::unordered_map<packet_id_t, uint64_t> packet_id_translation{};
};

// P4Runtime supports sending notifications to the client which are
// "traditionally" sent using nanomsg PUBSUB messages: table entry idle time
// notifications & learning notifications. simple_switch_grpc "captures" these
// notifications by providing a custom TransportIface implementation to the
// Switch base class. At the moment we capture all notifications (include port
// oper status notifications), but we only process (i.e. send through P4Runtime)
// learning notifications. This should not be a problem as users of
// simple_switch_grpc usually disable nanomsg anyway.
class NotificationsCapture : public bm::TransportIface {
 public:
  explicit NotificationsCapture(bm::SwitchWContexts *sw)
      : sw(sw) { }

 private:
  static constexpr size_t hdr_size = 32u;

  using Lock = std::lock_guard<std::mutex>;

  struct LEA_hdr_t {
    char sub_topic[4];
    uint64_t switch_id;
    uint32_t cxt_id;
    int list_id;
    uint64_t buffer_id;
    unsigned int num_samples;
  } __attribute__((packed));

  struct AGE_hdr_t {
    char sub_topic[4];
    uint64_t switch_id;
    uint32_t cxt_id;
    uint64_t buffer_id;
    int table_id;
    unsigned int num_entries;
  } __attribute__((packed));

  struct PRT_hdr_t {
    char sub_topic[4];
    uint64_t switch_id;
    unsigned int num_statuses;
    char _padding[16];
  } __attribute__((packed));

  struct SWP_hdr_t {
    char sub_topic[4];
    uint64_t switch_id;
    uint32_t cxt_id;
    int status;
    char _padding[12];
  } __attribute__((packed));

  static_assert(sizeof(LEA_hdr_t) == hdr_size,
                "Invalid size for notification header");
  static_assert(sizeof(AGE_hdr_t) == hdr_size,
                "Invalid size for notification header");
  static_assert(sizeof(PRT_hdr_t) == hdr_size,
                "Invalid size for notification header");
  static_assert(sizeof(SWP_hdr_t) == hdr_size,
                "Invalid size for notification header");

  int send_generic(const std::string &msg) const {
    if (msg.size() < hdr_size) return 1;
    // all notification headers have size 32 bytes, padded at the end if needed
    std::aligned_storage<hdr_size>::type storage;
    std::memcpy(&storage, msg.data(), sizeof(storage));
    const char *data = msg.data() + hdr_size;
    Lock lock(mutex);
    if (!memcmp("SWP|", msg.data(), 4)) {
      handle_SWP(reinterpret_cast<const SWP_hdr_t *>(&storage));
    } else if (!memcmp("LEA|", msg.data(), 4)) {
      handle_LEA(reinterpret_cast<const LEA_hdr_t *>(&storage),
                 data, msg.size() - hdr_size);
    } else if (!memcmp("AGE|", msg.data(), 4)) {
      handle_AGE(reinterpret_cast<const AGE_hdr_t *>(&storage),
                 data, msg.size() - hdr_size);
    }
    return 0;
  }

  // we use Swap notifications to ensure that learning & ageing notificaitons
  // are not sent to PI / P4Runtime during the config swap process, which is a
  // requirement of the p4lang P4Runtime implementation.
  void handle_SWP(const SWP_hdr_t *hdr) const {
    if (hdr->cxt_id != 0) {
      return;
    }
    enum SwapStatus {
      NEW_CONFIG_LOADED = 0,
      SWAP_REQUESTED = 1,
      SWAP_COMPLETED = 2,
      SWAP_CANCELLED = 3
    };
    if (static_cast<SwapStatus>(hdr->status) == NEW_CONFIG_LOADED) {
      ongoing_swap = true;
    } else if (static_cast<SwapStatus>(hdr->status) == SWAP_COMPLETED ||
               static_cast<SwapStatus>(hdr->status) == SWAP_CANCELLED) {
      ongoing_swap = false;
    }
  }

  void handle_LEA(const LEA_hdr_t *hdr, const char *data, size_t size) const {
    // do not send notifications to PI if there is an ongoing swap; this is a
    // requirement of the p4lang P4Runtime implementation.
    if (ongoing_swap) {
      BMLOG_TRACE(
          "Ignoring LEA notification because of ongoing dataplane swap");
      return;
    }
    const auto *learn_engine = sw->get_learn_engine(0);
    std::string list_name;
    if (learn_engine->list_get_name_from_id(hdr->list_id, &list_name) !=
        bm::LearnEngineIface::LearnErrorCode::SUCCESS) {
      bm::Logger::get()->error(
          "Ignoring LEA notification with unknown learn list id {}",
          hdr->list_id);
      return;
    }
    auto *p4info = pi_get_device_p4info(hdr->switch_id);
    if (p4info == nullptr) {
      bm::Logger::get()->error(
          "Ignoring LEA notification for device {} which has no p4info",
          hdr->switch_id);
      return;
    }
    pi_p4_id_t pi_id = pi_p4info_digest_id_from_name(p4info, list_name.c_str());
    if (pi_id == PI_INVALID_ID) {
      bm::Logger::get()->error(
          "Ignoring LEA notification whose name '{}' cannot be found in p4info",
          list_name);
      return;
    }
    size_t data_size = pi_p4info_digest_data_size(p4info, pi_id);
    if (data_size != size / hdr->num_samples) {
      bm::Logger::get()->error(
          "Dropping LEA notification with name '{}' because of unexpected "
          "digest size", list_name);
      return;
    }
    // Arguably this part of the code should be in PI/src/pi_learn_imp.cpp,
    // along with the pi_learn_msg_done implementation (which releases the
    // memory allocated here).
    pi_learn_msg_t *pi_msg = new pi_learn_msg_t;
    pi_msg->dev_tgt.dev_id = hdr->switch_id;
    pi_msg->dev_tgt.dev_pipe_mask = hdr->cxt_id;
    pi_msg->learn_id = pi_id;
    pi_msg->msg_id = hdr->buffer_id;
    pi_msg->num_entries = hdr->num_samples;
    pi_msg->entry_size = data_size;
    pi_msg->entries = new char[size];
    std::memcpy(pi_msg->entries, data, size);
    pi_learn_new_msg(pi_msg);
  }

  void handle_AGE(const AGE_hdr_t *hdr, const char *data, size_t size) const {
    (void) size;
    // do not send notifications to PI if there is an ongoing swap; this is a
    // requirement of the p4lang P4Runtime implementation.
    if (ongoing_swap) {
      BMLOG_TRACE(
          "Ignoring AGE notification because of ongoing dataplane swap");
      return;
    }

    const auto *ageing_monitor = sw->get_ageing_monitor(0);
    auto table_name = ageing_monitor->get_table_name_from_id(hdr->table_id);
    if (table_name == "") {
      bm::Logger::get()->error(
          "Ignoring AGE notification with unknown table id {}", hdr->table_id);
      return;
    }
    auto *p4info = pi_get_device_p4info(hdr->switch_id);
    if (p4info == nullptr) {
      bm::Logger::get()->error(
          "Ignoring AGE notification for device {} which has no p4info",
          hdr->switch_id);
      return;
    }
    pi_p4_id_t pi_id = pi_p4info_table_id_from_name(p4info, table_name.c_str());
    if (pi_id == PI_INVALID_ID) {
      bm::Logger::get()->error(
          "Ignoring AGE notification for table whose name '{}' "
          "cannot be found in p4info", table_name);
      return;
    }

    auto *handles = reinterpret_cast<const uint32_t *>(data);
    for (unsigned int i = 0; i < hdr->num_entries; i++) {
      bm::pi::table_idle_timeout_notify(
          hdr->switch_id, pi_id, static_cast<pi_entry_handle_t>(handles[i]));
    }
  }

  int open_() override {
    return 0;
  }

  int send_(const std::string &msg) const override {
    return send_generic(msg);
  }

  int send_(const char *msg, int len) const override {
    return send_generic(std::string(msg, len));
  }

  int send_msgs_(
      const std::initializer_list<std::string> &msgs) const override {
    std::string buf;
    for (const auto &msg : msgs) buf.append(msg);
    return send_generic(buf);
  }

  int send_msgs_(const std::initializer_list<MsgBuf> &msgs) const override {
    // TODO(antonin): since this is the method which is actually used by the
    // bm_sim library when generating notifications, it may make sense to
    // optimize the implementation for this case...
    std::string buf;
    for (const auto &msg : msgs) buf.append(msg.buf, msg.len);
    return send_generic(buf);
  }

  mutable std::mutex mutex{};
  mutable bool ongoing_swap{false};
  bm::SwitchWContexts *sw;
};


SimpleSwitchGrpcRunner::SimpleSwitchGrpcRunner(
    bool enable_swap,
    std::string grpc_server_addr,
    bm::DevMgrIface::port_t cpu_port,
    std::string dp_grpc_server_addr,
    bm::DevMgrIface::port_t drop_port)
    : simple_switch(new SimpleSwitch(enable_swap, drop_port)),
      grpc_server_addr(grpc_server_addr), cpu_port(cpu_port),
      dp_grpc_server_addr(dp_grpc_server_addr),
      dp_service(nullptr),
      dp_grpc_server(nullptr) {
  PIGrpcServerInit();
}

int
SimpleSwitchGrpcRunner::init_and_start(const bm::OptionsParser &parser) {
  std::unique_ptr<bm::DevMgrIface> my_dev_mgr = nullptr;
  if (!dp_grpc_server_addr.empty()) {
    dp_service = new DataplaneInterfaceServiceImpl(parser.device_id);
    grpc::ServerBuilder builder;
    builder.SetSyncServerOption(
      grpc::ServerBuilder::SyncServerOption::NUM_CQS, 1);
    builder.SetSyncServerOption(
      grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, 1);
    builder.SetSyncServerOption(
      grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, 1);
    builder.AddListeningPort(dp_grpc_server_addr,
                             grpc::InsecureServerCredentials(),
                             &dp_grpc_server_port);
    builder.RegisterService(dp_service);
    dp_grpc_server = builder.BuildAndStart();
    my_dev_mgr.reset(dp_service);
  }

#ifdef WITH_SYSREPO
      sysrepo_driver = std::unique_ptr<SysrepoDriver>(new SysrepoDriver(
          parser.device_id, simple_switch.get()));
#endif  // WITH_SYSREPO

  // Even when using gNMI to manage ports, it is convenient to be able to use
  // the --interface / -i command-line option. However we have to "intercept"
  // the options before the call to DevMgr::port_add, otherwise we would try to
  // add the ports twice (once when calling Switch::init_from_options_parser and
  // once when we are notified by sysrepo of the YANG datastore change).
  // We therefore save the interface list provided on the command-line and we
  // call SysrepoDriver::add_iface at the end of this method after we start the
  // sysrepo subscriber.
  const bm::OptionsParser *parser_ptr;
#ifdef WITH_SYSREPO
  auto new_parser = parser;
  auto &interfaces = new_parser.ifaces;
  auto saved_interfaces = interfaces;
  interfaces.clear();
  parser_ptr = &new_parser;
#else
  parser_ptr = &parser;
#endif  // WITH_SYSREPO

  auto my_transport = std::make_shared<NotificationsCapture>(
      simple_switch.get());
  int status = simple_switch->init_from_options_parser(
      *parser_ptr, std::move(my_transport), std::move(my_dev_mgr));
  if (status != 0) return status;

  if (parser.option_was_provided("notifications-addr")) {
    bm::Logger::get()->warn(
        "You provided --notifications-addr but this target captures all "
        "notifications and does not generate nanomsg messages");
  }

  // PortMonitor saves the CB by reference so we cannot use this code; it seems
  // that at this stage we do not need the CB any way.
  // using PortStatus = bm::DevMgrIface::PortStatus;
  // auto port_cb = std::bind(&SimpleSwitchGrpcRunner::port_status_cb, this,
  //                          std::placeholders::_1, std::placeholders::_2);
  // simple_switch->register_status_cb(PortStatus::PORT_ADDED, port_cb);
  // simple_switch->register_status_cb(PortStatus::PORT_REMOVED, port_cb);

  // check if CPU port number is also used by --interface
  // TODO(antonin): ports added dynamically?
  if (cpu_port > 0) {
    if (parser.ifaces.find(cpu_port) != parser.ifaces.end()) {
      bm::Logger::get()->error("Cpu port {} is used as a data port", cpu_port);
      return 1;
    }
  }

  auto transmit_fn = [this](bm::DevMgrIface::port_t port_num,
                            packet_id_t pkt_id, const char *buf, int len) {
    if (cpu_port > 0 && port_num == cpu_port) {
      BMLOG_DEBUG("Transmitting packet-in");
      auto status = pi_packetin_receive(simple_switch->get_device_id(),
                                        buf, static_cast<size_t>(len));
      if (status != PI_STATUS_SUCCESS)
        bm::Logger::get()->error("Error when transmitting packet-in");
    } else if (dp_service != nullptr) {
      // need pkt_id for gRPC dp service, so we have to call the my_transmit_fn
      // directly
      dp_service->my_transmit_fn(port_num, pkt_id, buf, len);
    } else {
      simple_switch->transmit_fn(port_num, buf, len);
    }
  };
  simple_switch->set_transmit_fn(transmit_fn);

  bm::pi::register_switch(simple_switch.get(), cpu_port);

  {
    using pi::fe::proto::LogWriterIface;
    using pi::fe::proto::LoggerConfig;
    class P4RuntimeLogger : public LogWriterIface {
      void write(Severity severity, const char *msg) override {
        auto severity_map = [&severity]() {
          namespace spdL = spdlog::level;
          switch (severity) {
            case Severity::TRACE : return spdL::trace;
            case Severity::DEBUG: return spdL::debug;
            case Severity::INFO: return spdL::info;
            case Severity::WARN: return spdL::warn;
            case Severity::ERROR: return spdL::err;
            case Severity::CRITICAL: return spdL::critical;
          }
          return spdL::off;
        };
        // TODO(antonin): use a separate logger with a separate name
        bm::Logger::get()->log(severity_map(), "[P4Runtime] {}", msg);
      }
    };
    LoggerConfig::set_writer(std::make_shared<P4RuntimeLogger>());
  }
  PIGrpcServerRunAddr(grpc_server_addr.c_str());

#ifdef WITH_SYSREPO
  if (!sysrepo_driver->start()) return 1;
  for (const auto &p : saved_interfaces)
    sysrepo_driver->add_iface(p.first, p.second);
#endif  // WITH_SYSREPO

#ifdef WITH_THRIFT
  int thrift_port = simple_switch->get_runtime_port();
  bm_runtime::start_server(simple_switch.get(), thrift_port);
  using ::sswitch_runtime::SimpleSwitchIf;
  using ::sswitch_runtime::SimpleSwitchProcessor;
  bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>(
          "simple_switch", sswitch_runtime::get_handler(simple_switch.get()));
#else
  if (parser.option_was_provided("thrift-port")) {
    bm::Logger::get()->warn(
        "You used the '--thrift-port' command-line option, but this target was "
        "compiled without Thrift support. You can enable Thrift support (not "
        "recommended) by providing '--with-thrift' to configure.");
  }
#endif  // WITH_THRIFT

  simple_switch->start_and_return();

  return 0;
}

void
SimpleSwitchGrpcRunner::wait() {
  PIGrpcServerWait();
}

void
SimpleSwitchGrpcRunner::shutdown() {
  if (!dp_grpc_server_addr.empty()) dp_grpc_server->Shutdown();
  PIGrpcServerShutdown();
}

void
SimpleSwitchGrpcRunner::block_until_all_packets_processed() {
  simple_switch->block_until_no_more_packets();
}

bool
SimpleSwitchGrpcRunner::is_dp_service_active() {
  if (dp_service != nullptr) {
    return dp_service->get_packet_stream_status();
  }
  return false;
}

SimpleSwitchGrpcRunner::~SimpleSwitchGrpcRunner() {
  PIGrpcServerCleanup();
}

void
SimpleSwitchGrpcRunner::port_status_cb(bm::DevMgrIface::port_t port,
    const bm::DevMgrIface::PortStatus port_status) {
  _BM_UNUSED(port);
  _BM_UNUSED(port_status);
}

}  // namespace sswitch_grpc
