#include "SimpleSwitch.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

#include <bm_sim/switch.h>

#include "simple_switch.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::sswitch_runtime;

class SimpleSwitchHandler : virtual public SimpleSwitchIf {
 public:
  SimpleSwitchHandler(Switch *sw)
    : switch_(dynamic_cast<SimpleSwitch *>(sw)) { }

  int32_t mirroring_mapping_add(const int32_t mirror_id, const int32_t egress_port) {
    printf("mirroring_mapping_add\n");
    return switch_->mirroring_mapping_add(mirror_id, egress_port);
  }

  int32_t mirroring_mapping_delete(const int32_t mirror_id) {
    printf("mirroring_mapping_delete\n");
    return switch_->mirroring_mapping_delete(mirror_id);
  }

  int32_t mirroring_mapping_get_egress_port(const int32_t mirror_id) {
    printf("mirroring_mapping_get_egress_port\n");
  }

  int32_t set_egress_queue_depth(const int32_t depth_pkts) {
    printf("set_egress_queue_depth\n");
    return switch_->set_egress_queue_depth(static_cast<size_t>(depth_pkts));
  }

  int32_t set_egress_queue_rate(const int64_t rate_pps) {
    printf("set_egress_queue_rate\n");
    return switch_->set_egress_queue_rate(static_cast<uint64_t>(rate_pps));
  }

private:
  SimpleSwitch *switch_;
};
