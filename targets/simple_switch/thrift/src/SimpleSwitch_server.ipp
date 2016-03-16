#include "SimpleSwitch.h"

#ifdef P4THRIFT
#include <p4thrift/protocol/TBinaryProtocol.h>
#include <p4thrift/server/TThreadedServer.h>
#include <p4thrift/transport/TServerSocket.h>
#include <p4thrift/transport/TBufferTransports.h>

namespace thrift_provider = p4::thrift;
#else
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

namespace thrift_provider = apache::thrift;
#endif

#include <bm_sim/switch.h>
#include <bm_sim/logger.h>

#include "simple_switch.h"

using namespace ::thrift_provider;
using namespace ::thrift_provider::protocol;
using namespace ::thrift_provider::transport;
using namespace ::thrift_provider::server;

using boost::shared_ptr;

using namespace  ::sswitch_runtime;

class SimpleSwitchHandler : virtual public SimpleSwitchIf {
 public:
  SimpleSwitchHandler(Switch *sw)
    : switch_(dynamic_cast<SimpleSwitch *>(sw)) { }

  int32_t mirroring_mapping_add(const int32_t mirror_id, const int32_t egress_port) {
    bm::Logger::get()->trace("mirroring_mapping_add");
    return switch_->mirroring_mapping_add(mirror_id, egress_port);
  }

  int32_t mirroring_mapping_delete(const int32_t mirror_id) {
    bm::Logger::get()->trace("mirroring_mapping_delete");
    return switch_->mirroring_mapping_delete(mirror_id);
  }

  int32_t mirroring_mapping_get_egress_port(const int32_t mirror_id) {
    bm::Logger::get()->trace("mirroring_mapping_get_egress_port");
    return switch_->mirroring_mapping_get(mirror_id);
  }

  int32_t set_egress_queue_depth(const int32_t depth_pkts) {
    bm::Logger::get()->trace("set_egress_queue_depth");
    return switch_->set_egress_queue_depth(static_cast<size_t>(depth_pkts));
  }

  int32_t set_egress_queue_rate(const int64_t rate_pps) {
    bm::Logger::get()->trace("set_egress_queue_rate");
    return switch_->set_egress_queue_rate(static_cast<uint64_t>(rate_pps));
  }

private:
  SimpleSwitch *switch_;
};
