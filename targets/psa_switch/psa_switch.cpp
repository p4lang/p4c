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

packet_id_t PsaSwitch::packet_id = 0;

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
    start(clock::now()) {
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
}

#define PACKET_LENGTH_REG_IDX 0

int
PsaSwitch::receive_(port_t port_num, const char *buffer, int len) {

  // for p4runtime program swap - antonin
  // putting do_swap call here is ok because blocking this thread will not
  // block processing of existing packet instances, which is a requirement
  do_swap();

  // we limit the packet buffer to original size + 512 bytes, which means we
  // cannot add more than 512 bytes of header data to the packet, which should
  // be more than enough
  auto packet = new_packet_ptr(port_num, packet_id++, len,
                               bm::PacketBuffer(len + 512, buffer, len));

  BMELOG(packet_in, *packet);
  PHV *phv = packet->get_phv();

  // many current p4 programs assume this
  // from psa spec - PSA does not mandate initialization of user-defined
  // metadata to known values as given as input to the ingress parser
  phv->reset_metadata();

  // TODO use appropriate enum member from JSON
  phv->get_field("psa_ingress_parser_input_metadata.packet_path").set(PKT_INSTANCE_TYPE_NORMAL);
  phv->get_field("psa_ingress_parser_input_metadata.ingress_port").set(port_num);

  // using packet register 0 to store length, this register will be updated for
  // each add_header / remove_header primitive call
  packet->set_register(PACKET_LENGTH_REG_IDX, len);

  phv->get_field("psa_ingress_input_metadata.ingress_timestamp")
    .set(get_ts().count());

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
    size_t priority = phv->has_field(SSWITCH_PRIORITY_QUEUEING_SRC) ?
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

// used for ingress cloning, resubmit
void
PsaSwitch::copy_field_list_and_set_type(
    const std::unique_ptr<Packet> &packet,
    const std::unique_ptr<Packet> &packet_copy,
    p4object_id_t field_list_id) {
  PHV *phv_copy = packet_copy->get_phv();
  phv_copy->reset_metadata();
  FieldList *field_list = this->get_field_list(field_list_id);
  field_list->copy_fields_between_phvs(phv_copy, packet->get_phv());
}

void
PsaSwitch::ingress_thread() {
  PHV *phv;

  while (1) {
    std::unique_ptr<Packet> packet;
    input_buffer.pop_back(&packet);
    if (packet == nullptr) break;

    Parser *parser = this->get_parser("ingress_parser");
    Pipeline *ingress_mau = this->get_pipeline("ingress");
    phv = packet->get_phv();

    port_t ingress_port = packet->get_ingress_port();
    (void) ingress_port;
    BMLOG_DEBUG_PKT(*packet, "Processing packet received on port {}",
                    ingress_port);

    parser->parse(packet.get());
    ingress_mau->apply(packet.get());

    // prioritize dropping if marked as such - do not move below other checks
    if (phv->has_field("psa_ingress_output_metadata.drop")) {
      Field &f_drop = phv->get_field("psa_ingress_output_metadata.drop");
      if (f_drop.get_int()) {
        continue;
      }
    }

    // handling multicast
    unsigned int mgid = 0u;
    if (phv->has_field("psa_ingress_output_metadata.multicast_group")) {
      Field &f_mgid = phv->get_field("psa_ingress_output_metadata.multicast_group");
      mgid = f_mgid.get_uint();

      if(mgid != 0){
        const auto pre_out = pre->replicate({mgid});
        auto packet_size = packet->get_register(PACKET_LENGTH_REG_IDX);
        for(const auto &out : pre_out){
          auto egress_port = out.egress_port;
          std::unique_ptr<Packet> packet_copy = packet->clone_with_phv_ptr();
          packet_copy->set_register(PACKET_LENGTH_REG_IDX, packet_size);
          enqueue(egress_port, std::move(packet_copy));
        }
        continue;
      }
    }

    packet->reset_exit();
    Field &f_egress_spec = phv->get_field("psa_ingress_output_metadata.egress_port");
    port_t egress_spec = f_egress_spec.get_uint();
    port_t egress_port = egress_spec;

    egress_port = egress_spec;
    BMLOG_DEBUG_PKT(*packet, "Egress port is {}", egress_port);

    Deparser *deparser = this->get_deparser("ingress_deparser");
    deparser->deparse(packet.get());
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

    Parser *parser = this->get_parser("egress_parser");
    parser->parse(packet.get());
    Deparser *deparser = this->get_deparser("egress_deparser");
    Pipeline *egress_mau = this->get_pipeline("egress");
    egress_mau->apply(packet.get());
    deparser->deparse(packet.get());

    if (port == PSA_PORT_RECIRCULATE) {
      BMLOG_DEBUG_PKT(*packet, "Recirculating packet");
      phv = packet->get_phv();

      phv->reset();
      phv->reset_header_stacks();
      phv->reset_metadata();

      phv->get_field("psa_ingress_parser_input_metadata.ingress_port")
        .set(PSA_PORT_RECIRCULATE);
      phv->get_field("psa_ingress_parser_input_metadata.packet_path")
        .set(PKT_INSTANCE_TYPE_RECIRC);
      phv->get_field("psa_ingress_input_metadata.ingress_port")
        .set(PSA_PORT_RECIRCULATE);
      phv->get_field("psa_ingress_input_metadata.packet_path")
        .set(PKT_INSTANCE_TYPE_RECIRC);
      phv->get_field("psa_ingress_input_metadata.ingress_timestamp")
        .set(get_ts().count());
      input_buffer.push_front(std::move(packet));
      continue;
    }
    output_buffer.push_front(std::move(packet));
  }
}
