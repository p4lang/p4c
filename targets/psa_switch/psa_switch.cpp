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

#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/logger.h>

#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>

#include "psa_switch.h"

namespace {

struct hash_ex {
  uint32_t operator()(const char *buf, size_t s) const {
    const uint32_t p = 16777619;
    uint32_t hash = 2166136261;

    for (size_t i = 0; i < s; i++)
      hash = (hash ^ buf[i]) * p;

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return static_cast<uint32_t>(hash);
  }
};

struct bmv2_hash {
  uint64_t operator()(const char *buf, size_t s) const {
    return bm::hash::xxh64(buf, s);
  }
};

}  // namespace

// if REGISTER_HASH calls placed in the anonymous namespace, some compiler can
// give an unused variable warning
REGISTER_HASH(hash_ex);
REGISTER_HASH(bmv2_hash);

extern int import_primitives();
extern int import_counters();
extern int import_meters();

namespace bm {

namespace psa {

static constexpr uint16_t MAX_MIRROR_SESSION_ID = (1u << 15) - 1;
packet_id_t PsaSwitch::packet_id = 0;

class PsaSwitch::MirroringSessions {
 public:
  bool add_session(mirror_id_t mirror_id,
                   const MirroringSessionConfig &config) {
    Lock lock(mutex);
    if (0 <= mirror_id && mirror_id <= MAX_MIRROR_SESSION_ID) {
      sessions_map[mirror_id] = config;
      return true;
    } else {
      bm::Logger::get()->error("mirror_id out of range. No session added.");
      return false;
    }
  }

  bool delete_session(mirror_id_t mirror_id) {
    Lock lock(mutex);
    if (0 <= mirror_id && mirror_id <= MAX_MIRROR_SESSION_ID) {
      return sessions_map.erase(mirror_id) == 1;
    } else {
      bm::Logger::get()->error("mirror_id out of range. No session deleted.");
      return false;
    }
  }

  bool get_session(mirror_id_t mirror_id,
                   MirroringSessionConfig *config) const {
    Lock lock(mutex);
    auto it = sessions_map.find(mirror_id);
    if (it == sessions_map.end()) return false;
    *config = it->second;
    return true;
  }

 private:
  using Mutex = std::mutex;
  using Lock = std::lock_guard<Mutex>;

  mutable std::mutex mutex;
  std::unordered_map<mirror_id_t, MirroringSessionConfig> sessions_map;
};

PsaSwitch::PsaSwitch(bool enable_swap)
  : Switch(enable_swap),
    input_buffer(1024),
#ifdef SSWITCH_PRIORITY_QUEUEING_ON
    egress_buffers(nb_egress_threads,
                   64, EgressThreadMapper(nb_egress_threads),
                   SSWITCH_PRIORITY_QUEUEING_NB_QUEUES),
#else
    egress_buffers(nb_egress_threads,
                   64, EgressThreadMapper(nb_egress_threads)),
#endif
    output_buffer(128),
    // cannot use std::bind because of a clang bug
    // https://stackoverflow.com/questions/32030141/is-this-incorrect-use-of-stdbind-or-a-compiler-bug
    my_transmit_fn([this](port_t port_num, packet_id_t pkt_id,
                          const char *buffer, int len) {
        _BM_UNUSED(pkt_id);
        this->transmit_fn(port_num, buffer, len);
    }),
    pre(new McSimplePreLAG()),
    start(clock::now()),
    mirroring_sessions(new MirroringSessions()) {
  add_component<McSimplePreLAG>(pre);

  add_required_field("psa_ingress_parser_input_metadata", "ingress_port");
  add_required_field("psa_ingress_parser_input_metadata", "packet_path");

  add_required_field("psa_ingress_input_metadata", "ingress_port");
  add_required_field("psa_ingress_input_metadata", "packet_path");
  add_required_field("psa_ingress_input_metadata", "ingress_timestamp");
  add_required_field("psa_ingress_input_metadata", "parser_error");

  add_required_field("psa_ingress_output_metadata", "class_of_service");
  add_required_field("psa_ingress_output_metadata", "clone");
  add_required_field("psa_ingress_output_metadata", "clone_session_id");
  add_required_field("psa_ingress_output_metadata", "drop");
  add_required_field("psa_ingress_output_metadata", "resubmit");
  add_required_field("psa_ingress_output_metadata", "multicast_group");
  add_required_field("psa_ingress_output_metadata", "egress_port");

  add_required_field("psa_egress_parser_input_metadata", "egress_port");
  add_required_field("psa_egress_parser_input_metadata", "packet_path");

  add_required_field("psa_egress_input_metadata", "class_of_service");
  add_required_field("psa_egress_input_metadata", "egress_port");
  add_required_field("psa_egress_input_metadata", "packet_path");
  add_required_field("psa_egress_input_metadata", "instance");
  add_required_field("psa_egress_input_metadata", "egress_timestamp");
  add_required_field("psa_egress_input_metadata", "parser_error");

  add_required_field("psa_egress_output_metadata", "clone");
  add_required_field("psa_egress_output_metadata", "clone_session_id");
  add_required_field("psa_egress_output_metadata", "drop");

  add_required_field("psa_egress_deparser_input_metadata", "egress_port");

  force_arith_header("psa_ingress_parser_input_metadata");
  force_arith_header("psa_ingress_input_metadata");
  force_arith_header("psa_ingress_output_metadata");
  force_arith_header("psa_egress_parser_input_metadata");
  force_arith_header("psa_egress_input_metadata");
  force_arith_header("psa_egress_output_metadata");
  force_arith_header("psa_egress_deparser_input_metadata");

  import_primitives();
  import_counters();
  import_meters();
}

#define PACKET_LENGTH_REG_IDX 0

int
PsaSwitch::receive_(port_t port_num, const char *buffer, int len) {
  // we limit the packet buffer to original size + 512 bytes, which means we
  // cannot add more than 512 bytes of header data to the packet, which should
  // be more than enough
  auto packet = new_packet_ptr(port_num, packet_id++, len,
                               bm::PacketBuffer(len + 512, buffer, len));

  BMELOG(packet_in, *packet);
  auto *phv = packet->get_phv();

  // many current p4 programs assume this
  // from psa spec - PSA does not mandate initialization of user-defined
  // metadata to known values as given as input to the ingress parser
  phv->reset_metadata();

  // TODO use appropriate enum member from JSON
  phv->get_field("psa_ingress_parser_input_metadata.packet_path").set(PACKET_PATH_NORMAL);
  phv->get_field("psa_ingress_parser_input_metadata.ingress_port").set(port_num);

  // using packet register 0 to store length, this register will be updated for
  // each add_header / remove_header primitive call
  packet->set_register(PACKET_LENGTH_REG_IDX, len);

  input_buffer.push_front(std::move(packet));
  return 0;
}

void
PsaSwitch::start_and_return_() {
  threads_.push_back(std::thread(&PsaSwitch::ingress_thread, this));
  for (size_t i = 0; i < nb_egress_threads; i++) {
    threads_.push_back(std::thread(&PsaSwitch::egress_thread, this, i));
  }
  threads_.push_back(std::thread(&PsaSwitch::transmit_thread, this));
}

PsaSwitch::~PsaSwitch() {
  input_buffer.push_front(nullptr);
  for (size_t i = 0; i < nb_egress_threads; i++) {
#ifdef SSWITCH_PRIORITY_QUEUEING_ON
    egress_buffers.push_front(i, 0, nullptr);
#else
    egress_buffers.push_front(i, nullptr);
#endif
  }
  output_buffer.push_front(nullptr);
  for (auto& thread_ : threads_) {
    thread_.join();
  }
}

void
PsaSwitch::reset_target_state_() {
  bm::Logger::get()->debug("Resetting psa_switch target-specific state");
  get_component<McSimplePreLAG>()->reset_state();
}

bool
PsaSwitch::mirroring_add_session(mirror_id_t mirror_id,
                                    const MirroringSessionConfig &config) {
  return mirroring_sessions->add_session(mirror_id, config);
}

bool
PsaSwitch::mirroring_delete_session(mirror_id_t mirror_id) {
  return mirroring_sessions->delete_session(mirror_id);
}

bool
PsaSwitch::mirroring_get_session(mirror_id_t mirror_id,
                                    MirroringSessionConfig *config) const {
  return mirroring_sessions->get_session(mirror_id, config);
}

int
PsaSwitch::set_egress_queue_depth(size_t port, const size_t depth_pkts) {
  egress_buffers.set_capacity(port, depth_pkts);
  return 0;
}

int
PsaSwitch::set_all_egress_queue_depths(const size_t depth_pkts) {
  egress_buffers.set_capacity_for_all(depth_pkts);
  return 0;
}

int
PsaSwitch::set_egress_queue_rate(size_t port, const uint64_t rate_pps) {
  egress_buffers.set_rate(port, rate_pps);
  return 0;
}

int
PsaSwitch::set_all_egress_queue_rates(const uint64_t rate_pps) {
  egress_buffers.set_rate_for_all(rate_pps);
  return 0;
}

uint64_t
PsaSwitch::get_time_elapsed_us() const {
  return get_ts().count();
}

uint64_t
PsaSwitch::get_time_since_epoch_us() const {
  auto tp = clock::now();
  return duration_cast<ts_res>(tp.time_since_epoch()).count();
}

void
PsaSwitch::set_transmit_fn(TransmitFn fn) {
  my_transmit_fn = std::move(fn);
}

void
PsaSwitch::transmit_thread() {
  while (1) {
    std::unique_ptr<Packet> packet;
    output_buffer.pop_back(&packet);

    if (packet == nullptr) break;
    BMELOG(packet_out, *packet);
    BMLOG_DEBUG_PKT(*packet, "Transmitting packet of size {} out of port {}",
                    packet->get_data_size(), packet->get_egress_port());

    my_transmit_fn(packet->get_egress_port(), packet->get_packet_id(),
                   packet->data(), packet->get_data_size());
  }
}

ts_res
PsaSwitch::get_ts() const {
  return duration_cast<ts_res>(clock::now() - start);
}

void
PsaSwitch::enqueue(port_t egress_port, std::unique_ptr<Packet> &&packet) {
    packet->set_egress_port(egress_port);

#ifdef SSWITCH_PRIORITY_QUEUEING_ON
    auto priority = phv->has_field(SSWITCH_PRIORITY_QUEUEING_SRC) ?
        phv->get_field(SSWITCH_PRIORITY_QUEUEING_SRC).get<size_t>() : 0u;
    if (priority >= SSWITCH_PRIORITY_QUEUEING_NB_QUEUES) {
      bm::Logger::get()->error("Priority out of range, dropping packet");
      return;
    }
    egress_buffers.push_front(
        egress_port, SSWITCH_PRIORITY_QUEUEING_NB_QUEUES - 1 - priority,
        std::move(packet));
#else
    egress_buffers.push_front(egress_port, std::move(packet));
#endif
}

void
PsaSwitch::multicast(Packet *packet, unsigned int mgid, PktInstanceType path, unsigned int class_of_service) {
  auto phv = packet->get_phv();
  const auto pre_out = pre->replicate({mgid});
  auto &f_eg_cos = phv->get_field("psa_egress_input_metadata.class_of_service");
  auto &f_instance = phv->get_field("psa_egress_input_metadata.instance");
  auto &f_packet_path = phv->get_field("psa_egress_parser_input_metadata.packet_path");
  auto packet_size = packet->get_register(PACKET_LENGTH_REG_IDX);
  for (const auto &out : pre_out) {
    auto egress_port = out.egress_port;
    auto instance = out.rid;
    BMLOG_DEBUG_PKT(*packet,
                    "Replicating packet on port {} with instance {}",
                    egress_port, instance);
    f_eg_cos.set(class_of_service);
    f_instance.set(instance);
    // TODO use appropriate enum member from JSON
    f_packet_path.set(path);
    std::unique_ptr<Packet> packet_copy = packet->clone_with_phv_ptr();
    packet_copy->set_register(PACKET_LENGTH_REG_IDX, packet_size);
    enqueue(egress_port, std::move(packet_copy));
  }
}

void
PsaSwitch::ingress_thread() {
  PHV *phv;

  while (1) {
    std::unique_ptr<Packet> packet;
    input_buffer.pop_back(&packet);
    if (packet == nullptr) break;

    phv = packet->get_phv();
    auto ingress_port =
        phv->get_field("psa_ingress_parser_input_metadata.ingress_port").
            get_uint();
    BMLOG_DEBUG_PKT(*packet, "Processing packet received on port {}",
                    ingress_port);

    /* Ingress cloning and resubmitting work on the packet before parsing.
       `buffer_state` contains the `data_size` field which tracks how many
       bytes are parsed by the parser ("lifted" into p4 headers). Here, we
       track the buffer_state prior to parsing so that we can put it back
       for packets that are cloned or resubmitted, same as in simple_switch.cpp
    */
    const Packet::buffer_state_t packet_in_state = packet->save_buffer_state();
    auto ingress_packet_size = packet->get_register(PACKET_LENGTH_REG_IDX);

    // The PSA specification says that for all packets, whether they
    // are new ones from a port, or resubmitted, or recirculated, the
    // ingress_timestamp should be the time near when the packet began
    // ingress processing.  This one place for assigning a value to
    // ingress_timestamp covers all cases.
    phv->get_field("psa_ingress_input_metadata.ingress_timestamp").set(
        get_ts().count());

    Parser *parser = this->get_parser("ingress_parser");
    parser->parse(packet.get());

    // pass relevant values from ingress parser
    // ingress_timestamp is already set above
    phv->get_field("psa_ingress_input_metadata.ingress_port").set(
        phv->get_field("psa_ingress_parser_input_metadata.ingress_port"));
    phv->get_field("psa_ingress_input_metadata.packet_path").set(
        phv->get_field("psa_ingress_parser_input_metadata.packet_path"));
    phv->get_field("psa_ingress_input_metadata.parser_error").set(
        packet->get_error_code().get());

    // set default metadata values according to PSA specification
    phv->get_field("psa_ingress_output_metadata.class_of_service").set(0);
    phv->get_field("psa_ingress_output_metadata.clone").set(0);
    phv->get_field("psa_ingress_output_metadata.drop").set(1);
    phv->get_field("psa_ingress_output_metadata.resubmit").set(0);
    phv->get_field("psa_ingress_output_metadata.multicast_group").set(0);

    Pipeline *ingress_mau = this->get_pipeline("ingress");
    ingress_mau->apply(packet.get());
    packet->reset_exit();

    const auto &f_ig_cos = phv->get_field("psa_ingress_output_metadata.class_of_service");
    const auto ig_cos = f_ig_cos.get_uint();

    // ingress cloning - each cloned packet is a copy of the packet as it entered the ingress parser
    //                 - dropped packets should still be cloned - do not move below drop
    auto clone = phv->get_field("psa_ingress_output_metadata.clone").get_uint();
    if (clone) {
      MirroringSessionConfig config;
      auto clone_session_id = phv->get_field("psa_ingress_output_metadata.clone_session_id").get<mirror_id_t>();
      auto is_session_configured = mirroring_get_session(clone_session_id, &config);

      if (is_session_configured) {
        BMLOG_DEBUG_PKT(*packet, "Cloning packet at ingress to session id {}", clone_session_id);
        const Packet::buffer_state_t packet_out_state = packet->save_buffer_state();
        packet->restore_buffer_state(packet_in_state);

        std::unique_ptr<Packet> packet_copy = packet->clone_no_phv_ptr();
        packet_copy->set_register(PACKET_LENGTH_REG_IDX, ingress_packet_size);
        auto phv_copy = packet_copy->get_phv();
        phv_copy->reset_metadata();
        phv_copy->get_field("psa_egress_parser_input_metadata.packet_path").set(PACKET_PATH_CLONE_I2E);

        if (config.mgid_valid) {
          BMLOG_DEBUG_PKT(*packet_copy, "Cloning packet to multicast group {}", config.mgid);
          // TODO 0 as the last arg (for class_of_service) is currently a placeholder
          // implement cos into cloning session configs
          multicast(packet_copy.get(), config.mgid, PACKET_PATH_CLONE_I2E, 0);
        }

        if (config.egress_port_valid) {
          BMLOG_DEBUG_PKT(*packet_copy, "Cloning packet to egress port {}", config.egress_port);
          enqueue(config.egress_port, std::move(packet_copy));
        }

        packet->restore_buffer_state(packet_out_state);
      } else {
        BMLOG_DEBUG_PKT(*packet,
                        "Cloning packet at ingress to unconfigured session id {} causes no clone packets to be created",
                        clone_session_id);
      }
    }

    // drop - packets marked via the ingress_drop action
    auto drop = phv->get_field("psa_ingress_output_metadata.drop").get_uint();
    if (drop) {
      BMLOG_DEBUG_PKT(*packet, "Dropping packet at the end of ingress");
      continue;
    }

    // resubmit - these packets get immediately resub'd to ingress, and skip
    //            deparsing, do not move below multicast or deparse
    auto resubmit = phv->get_field("psa_ingress_output_metadata.resubmit").get_uint();
    if (resubmit) {
      BMLOG_DEBUG_PKT(*packet, "Resubmitting packet");

      packet->restore_buffer_state(packet_in_state);
      phv->reset_metadata();
      phv->get_field("psa_ingress_parser_input_metadata.packet_path").set(
          PACKET_PATH_RESUBMIT);

      input_buffer.push_front(std::move(packet));
      continue;
    }

    Deparser *deparser = this->get_deparser("ingress_deparser");
    deparser->deparse(packet.get());

    auto &f_packet_path = phv->get_field("psa_egress_parser_input_metadata.packet_path");

    auto mgid = phv->get_field("psa_ingress_output_metadata.multicast_group").get_uint();
    if (mgid != 0) {
      BMLOG_DEBUG_PKT(*packet,
                      "Multicast requested for packet with multicast group {}",
                      mgid);
      multicast(packet.get(), mgid, PACKET_PATH_NORMAL_MULTICAST, ig_cos);
      continue;
    }

    auto &f_instance = phv->get_field("psa_egress_input_metadata.instance");
    auto &f_eg_cos = phv->get_field("psa_egress_input_metadata.class_of_service");
    f_instance.set(0);
    // TODO use appropriate enum member from JSON
    f_eg_cos.set(ig_cos);

    f_packet_path.set(PACKET_PATH_NORMAL_UNICAST);
    auto egress_port = phv->get_field("psa_ingress_output_metadata.egress_port").get<port_t>();
    BMLOG_DEBUG_PKT(*packet, "Egress port is {}", egress_port);
    enqueue(egress_port, std::move(packet));
  }
}

void
PsaSwitch::egress_thread(size_t worker_id) {
  PHV *phv;

  while (1) {
    std::unique_ptr<Packet> packet;
    size_t port;

#ifdef SSWITCH_PRIORITY_QUEUEING_ON
    size_t priority;
    egress_buffers.pop_back(worker_id, &port, &priority, &packet);
#else
    egress_buffers.pop_back(worker_id, &port, &packet);
#endif

    if (packet == nullptr) break;
    phv = packet->get_phv();

    // this reset() marks all headers as invalid - this is important since PSA
    // deparses packets after ingress processing - so no guarantees can be made
    // about their existence or validity while entering egress processing
    phv->reset();

    phv->get_field("psa_egress_parser_input_metadata.egress_port").set(port);
    phv->get_field("psa_egress_input_metadata.egress_timestamp").set(
        get_ts().count());

    Parser *parser = this->get_parser("egress_parser");
    parser->parse(packet.get());

    phv->get_field("psa_egress_input_metadata.egress_port").set(
        phv->get_field("psa_egress_parser_input_metadata.egress_port"));
    phv->get_field("psa_egress_input_metadata.packet_path").set(
        phv->get_field("psa_egress_parser_input_metadata.packet_path"));
    phv->get_field("psa_egress_input_metadata.parser_error").set(
        packet->get_error_code().get());

    // default egress output values according to PSA spec
    // clone_session_id is undefined by default
    phv->get_field("psa_egress_output_metadata.clone").set(0);
    phv->get_field("psa_egress_output_metadata.drop").set(0);

    Pipeline *egress_mau = this->get_pipeline("egress");
    egress_mau->apply(packet.get());
    packet->reset_exit();
    // TODO(peter): add stf test where exit is invoked but packet still gets recirc'd
    phv->get_field("psa_egress_deparser_input_metadata.egress_port").set(
        phv->get_field("psa_egress_parser_input_metadata.egress_port"));

    Deparser *deparser = this->get_deparser("egress_deparser");
    deparser->deparse(packet.get());

    // egress cloning - each cloned packet is a copy of the packet as output by the egress deparser
    auto clone = phv->get_field("psa_egress_output_metadata.clone").get_uint();
    if (clone) {
      MirroringSessionConfig config;
      auto clone_session_id = phv->get_field("psa_egress_output_metadata.clone_session_id").get<mirror_id_t>();
      auto is_session_configured = mirroring_get_session(clone_session_id, &config);

      if (is_session_configured) {
        BMLOG_DEBUG_PKT(*packet, "Cloning packet after egress to session id {}", clone_session_id);
        std::unique_ptr<Packet> packet_copy = packet->clone_no_phv_ptr();
        auto phv_copy = packet_copy->get_phv();
        phv_copy->reset_metadata();
        phv_copy->get_field("psa_egress_parser_input_metadata.packet_path").set(PACKET_PATH_CLONE_E2E);

        if (config.mgid_valid) {
          BMLOG_DEBUG_PKT(*packet_copy, "Cloning packet to multicast group {}", config.mgid);
          // TODO 0 as the last arg (for class_of_service) is currently a placeholder
          // implement cos into cloning session configs
          multicast(packet_copy.get(), config.mgid, PACKET_PATH_CLONE_E2E, 0);
        }

        if (config.egress_port_valid) {
          BMLOG_DEBUG_PKT(*packet_copy, "Cloning packet to egress port {}", config.egress_port);
          enqueue(config.egress_port, std::move(packet_copy));
        }

      } else {
        BMLOG_DEBUG_PKT(*packet,
                        "Cloning packet after egress to unconfigured session id {} causes no clone packets to be created",
                        clone_session_id);
      }
    }

    auto drop = phv->get_field("psa_egress_output_metadata.drop").get_uint();
    if (drop) {
      BMLOG_DEBUG_PKT(*packet, "Dropping packet at the end of egress");
      continue;
    }

    if (port == PSA_PORT_RECIRCULATE) {
      BMLOG_DEBUG_PKT(*packet, "Recirculating packet");

      phv->reset();
      phv->reset_header_stacks();
      phv->reset_metadata();

      phv->get_field("psa_ingress_parser_input_metadata.ingress_port")
        .set(PSA_PORT_RECIRCULATE);
      phv->get_field("psa_ingress_parser_input_metadata.packet_path")
        .set(PACKET_PATH_RECIRCULATE);
      input_buffer.push_front(std::move(packet));
      continue;
    }
    output_buffer.push_front(std::move(packet));
  }
}

}  // namespace bm::psa

}  // namespace bm
