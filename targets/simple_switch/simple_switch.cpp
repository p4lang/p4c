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

#include <iostream>
#include <fstream>
#include <string>

#include <unistd.h>

#include "bm_sim/parser.h"
#include "bm_sim/tables.h"

#include "primitives.h"
#include "simplelog.h"

#include "simple_switch.h"

#include "bm_runtime/bm_runtime.h"

#include "SimpleSwitch_server.ipp"

namespace {

struct hash_ex {
  uint32_t operator()(const char *buf, size_t s) const {
    const int p = 16777619;
    int hash = 2166136261;

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

REGISTER_HASH(hash_ex);

struct bmv2_hash {
  uint64_t operator()(const char *buf, size_t s) const {
    return hash::xxh64(buf, s);
  }
};

REGISTER_HASH(bmv2_hash);

}

SimpleSwitch::SimpleSwitch(int max_port)
  : Switch(false), // enable_switch = false
    max_port(max_port),
    input_buffer(1024), egress_buffers(max_port), output_buffer(128),
    pre(new McSimplePreLAG()),
    start(clock::now()) {
  for(int i = 0; i < max_port; i++) {
    egress_buffers[i].set_capacity(64);
  }

  add_component<McSimplePreLAG>(pre);

  add_required_field("standard_metadata", "ingress_port");
  add_required_field("standard_metadata", "packet_length");
  add_required_field("standard_metadata", "instance_type");
  add_required_field("standard_metadata", "egress_spec");
  add_required_field("standard_metadata", "clone_spec");
}

void SimpleSwitch::start_and_return() {
  std::thread t1(&SimpleSwitch::ingress_thread, this);
  t1.detach();
  for(int i = 0; i < max_port; i++) {
    std::thread t2(&SimpleSwitch::egress_thread, this, i);
    t2.detach();
  }
  std::thread t3(&SimpleSwitch::transmit_thread, this);
  t3.detach();
}

void SimpleSwitch::transmit_thread() {
  while(1) {
    std::unique_ptr<Packet> packet;
    output_buffer.pop_back(&packet);
    ELOGGER->packet_out(*packet);
    SIMPLELOG << "transmitting packet " << packet->get_packet_id() << std::endl;
    transmit_fn(packet->get_egress_port(), packet->data(), packet->get_data_size());
  }
}

ts_res SimpleSwitch::get_ts() const {
  return duration_cast<ts_res>(clock::now() - start);
}

void SimpleSwitch::enqueue(int egress_port, std::unique_ptr<Packet> &&packet) {
    packet->set_egress_port(egress_port);

    PHV *phv = packet->get_phv();

    if(phv->has_header("queueing_metadata")) {
      phv->get_field("queueing_metadata.enq_timestamp").set(get_ts().count());
      phv->get_field("queueing_metadata.enq_qdepth")
	.set(egress_buffers[egress_port].size());
    }

    egress_buffers[egress_port].push_front(std::move(packet));
}

void SimpleSwitch::ingress_thread() {
  Parser *parser = this->get_parser("parser");
  Pipeline *ingress_mau = this->get_pipeline("ingress");

  PHV *phv;

  while(1) {
    std::unique_ptr<Packet> packet;
    input_buffer.pop_back(&packet);

    phv = packet->get_phv();

    int ingress_port = packet->get_ingress_port();
    SIMPLELOG << "processing packet " << packet->get_packet_id()
	      << " received on port "<< ingress_port << std::endl;

    if(phv->has_field("intrinsic_metadata.ingress_global_timestamp")) {
      phv->get_field("intrinsic_metadata.ingress_global_timestamp")
        .set(get_ts().count());
    }

    // setting standard metadata
    phv->get_field("standard_metadata.ingress_port").set(ingress_port);
    int ingress_length = packet->get_ingress_length();
    phv->get_field("standard_metadata.packet_length").set(ingress_length);
    Field &f_instance_type = phv->get_field("standard_metadata.instance_type");
    f_instance_type.set(PKT_INSTANCE_TYPE_NORMAL);

    /* This looks like it comes out of the blue. However this is needed for
       ingress cloning. The parser updates the buffer state (pops the parsed
       headers) to make the deparser's job easier (the same buffer is
       re-used). But for ingress cloning, the original packet is needed. This
       kind of looks hacky though. Maybe a better solution would be to have the
       parser leave the buffer unchanged, and move the pop logic to the
       deparser. TODO? */
    const Packet::buffer_state_t packet_in_state = packet->save_buffer_state();
    parser->parse(packet.get());

    ingress_mau->apply(packet.get());

    Field &f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    int egress_spec = f_egress_spec.get_int();

    Field &f_clone_spec = phv->get_field("standard_metadata.clone_spec");
    unsigned int clone_spec = f_clone_spec.get_uint();

    int learn_id = 0;
    unsigned int mgid = 0u;

    if(phv->has_header("intrinsic_metadata")) {
      Field &f_learn_id = phv->get_field("intrinsic_metadata.lf_field_list");
      learn_id = f_learn_id.get_int();

      Field &f_mgid = phv->get_field("intrinsic_metadata.mcast_grp");
      mgid = f_mgid.get_uint();
    }

    packet_id_t copy_id;
    int egress_port;

    // INGRESS CLONING
    if(clone_spec) {
      SIMPLELOG << "cloning packet at ingress" << std::endl;
      egress_port = get_mirroring_mapping(clone_spec & 0xFFFF);
      if(egress_port >= 0) {
	const Packet::buffer_state_t packet_out_state = packet->save_buffer_state();
	packet->restore_buffer_state(packet_in_state);
	f_instance_type.set(PKT_INSTANCE_TYPE_INGRESS_CLONE);
	f_clone_spec.set(0);
	p4object_id_t field_list_id = clone_spec >> 16;
	copy_id = copy_id_dis(gen);
	std::unique_ptr<Packet> packet_copy(new Packet(packet->clone_no_phv(copy_id)));
	PHV *phv_copy = packet_copy->get_phv();
	phv_copy->reset_metadata();
	FieldList *field_list = this->get_field_list(field_list_id);
	for(const auto &p : *field_list) {
	  phv_copy->get_field(p.first, p.second)
	    .set(phv->get_field(p.first, p.second));
	}
	// we need to parse again
	// the alternative would be to pay the (huge) price of PHV copy for
	// every ingress packet
	parser->parse(packet_copy.get());
	enqueue(egress_port, std::move(packet_copy));
	f_instance_type.set(PKT_INSTANCE_TYPE_NORMAL);
	packet->restore_buffer_state(packet_out_state);
      }
    }
    
    // LEARNING
    if(learn_id > 0) {
      get_learn_engine()->learn(learn_id, *packet.get());
    }

    // MULTICAST
    int instance_type = f_instance_type.get_int();
    if(mgid != 0) {
      SIMPLELOG << "multicast\n";
      Field &f_rid = phv->get_field("intrinsic_metadata.egress_rid");
      const auto pre_out = pre->replicate({mgid});
      for(const auto &out : pre_out) {
	egress_port = out.egress_port;
	// if(ingress_port == egress_port) continue; // pruning
	SIMPLELOG << "replicating packet out of port " << egress_port
		  << std::endl;
	f_rid.set(out.rid);
	f_instance_type.set(PKT_INSTANCE_TYPE_REPLICATION);
	copy_id = copy_id_dis(gen);
	std::unique_ptr<Packet> packet_copy(new Packet(packet->clone(copy_id++)));
	enqueue(egress_port, std::move(packet_copy));
      }
      f_instance_type.set(instance_type);

      // when doing multicast, we discard the original packet
      continue;
    }

    egress_port = egress_spec;
    SIMPLELOG << "egress port is " << egress_port << std::endl;    

    if(egress_port == 511) {  // drop packet
      SIMPLELOG << "dropping packet\n";
      continue;
    }

    enqueue(egress_port, std::move(packet));
  }
}

void SimpleSwitch::egress_thread(int port) {
  Deparser *deparser = this->get_deparser("deparser");
  Pipeline *egress_mau = this->get_pipeline("egress");
  PHV *phv;

  while(1) {
    std::unique_ptr<Packet> packet;
    egress_buffers[port].pop_back(&packet);

    phv = packet->get_phv();

    if(phv->has_header("queueing_metadata")) {
      phv->get_field("queueing_metadata.deq_timestamp").set(get_ts().count());
      phv->get_field("queueing_metadata.deq_qdepth")
	.set(egress_buffers[port].size());
    }

    phv->get_field("standard_metadata.egress_port").set(port);

    Field &f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    f_egress_spec.set(0);

    egress_mau->apply(packet.get());

    Field &f_instance_type = phv->get_field("standard_metadata.instance_type");

    Field &f_clone_spec = phv->get_field("standard_metadata.clone_spec");
    unsigned int clone_spec = f_clone_spec.get_uint();

    packet_id_t copy_id;

    // EGRESS CLONING
    if(clone_spec) {
      SIMPLELOG << "cloning packet at egress" << std::endl;
      int egress_port = get_mirroring_mapping(clone_spec & 0xFFFF);
      if(egress_port >= 0) {
	f_instance_type.set(PKT_INSTANCE_TYPE_EGRESS_CLONE);
	f_clone_spec.set(0);
	p4object_id_t field_list_id = clone_spec >> 16;
	copy_id = copy_id_dis(gen);
	std::unique_ptr<Packet> packet_copy(new Packet(packet->clone_and_reset_metadata(copy_id++)));
	PHV *phv_copy = packet_copy->get_phv();
	FieldList *field_list = this->get_field_list(field_list_id);
	for(const auto &p : *field_list) {
	  phv_copy->get_field(p.first, p.second)
	    .set(phv->get_field(p.first, p.second));
	}
	enqueue(egress_port, std::move(packet_copy));
	f_instance_type.set(PKT_INSTANCE_TYPE_NORMAL);
      }
    }

    // TODO: should not be done like this in egress pipeline
    int egress_spec = f_egress_spec.get_int();
    if(egress_spec == 511) {  // drop packet
      SIMPLELOG << "dropping packet\n";
      continue;
    }

    deparser->deparse(packet.get());

    output_buffer.push_front(std::move(packet));
  }
}

/* Switch instance */

static SimpleSwitch *simple_switch;


int 
main(int argc, char* argv[])
{
  simple_switch = new SimpleSwitch();
  int status = simple_switch->init_from_command_line_options(argc, argv);
  if(status != 0) std::exit(status);

  int thrift_port = simple_switch->get_runtime_port();
  bm_runtime::start_server(simple_switch, thrift_port);
  bm_runtime::add_service<SimpleSwitchHandler, SimpleSwitchProcessor>("simple_switch");
  simple_switch->start_and_return();

  while(1) std::this_thread::sleep_for(std::chrono::seconds(100));
  
  return 0; 
}
