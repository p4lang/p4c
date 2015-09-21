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
#include <memory>
#include <thread>
#include <fstream>
#include <string>
#include <chrono>

#include <unistd.h>

#include "bm_sim/queue.h"
#include "bm_sim/packet.h"
#include "bm_sim/parser.h"
#include "bm_sim/tables.h"
#include "bm_sim/switch.h"
#include "bm_sim/event_logger.h"

#include "simple_router.h"
#include "primitives.h"
#include "simplelog.h"

#include "bm_runtime/bm_runtime.h"

// #define PCAP_DUMP

class SimpleSwitch : public Switch {
public:
  SimpleSwitch()
    : Switch(true), // enable_switch = true
      input_buffer(1024), output_buffer(128) { }

  int receive(int port_num, const char *buffer, int len) {
    static int pkt_id = 0;

    Packet *packet =
      new Packet(port_num, pkt_id++, 0, len, PacketBuffer(2048, buffer, len));

    ELOGGER->packet_in(*packet);

    input_buffer.push_front(std::unique_ptr<Packet>(packet));
    return 0;
  }

  void start_and_return() {
    std::thread t1(&SimpleSwitch::pipeline_thread, this);
    t1.detach();
    std::thread t2(&SimpleSwitch::transmit_thread, this);
    t2.detach();
  }

private:
  void pipeline_thread();
  void transmit_thread();

private:
  Queue<std::unique_ptr<Packet> > input_buffer;
  Queue<std::unique_ptr<Packet> > output_buffer;
};

void SimpleSwitch::transmit_thread() {
  while(1) {
    std::unique_ptr<Packet> packet;
    output_buffer.pop_back(&packet);
    ELOGGER->packet_out(*packet);
    SIMPLELOG << "transmitting packet " << packet->get_packet_id() << std::endl;
    transmit_fn(packet->get_egress_port(), packet->data(), packet->get_data_size());
  }
}

void SimpleSwitch::pipeline_thread() {
  Pipeline *ingress_mau = this->get_pipeline("ingress");
  Pipeline *egress_mau = this->get_pipeline("egress");
  Parser *parser = this->get_parser("parser");
  Deparser *deparser = this->get_deparser("deparser");
  PHV *phv;

  while(1) {
    std::unique_ptr<Packet> packet;
    input_buffer.pop_back(&packet);
    phv = packet->get_phv();
    SIMPLELOG << "processing packet " << packet->get_packet_id() << std::endl;

    // swap is enabled, so update pointers if needed
    if(this->do_swap() == 0) { // a swap took place
      ingress_mau = this->get_pipeline("ingress");
      egress_mau = this->get_pipeline("egress");
      parser = this->get_parser("parser");
      deparser = this->get_deparser("deparser");
    }
    
    parser->parse(packet.get());
    ingress_mau->apply(packet.get());

    int egress_port = phv->get_field("standard_metadata.egress_port").get_int();
    SIMPLELOG << "egress port is " << egress_port << std::endl;    

    if(egress_port == 511) {
      SIMPLELOG << "dropping packet\n";
    }
    else {
      packet->set_egress_port(egress_port);
      egress_mau->apply(packet.get());
      deparser->deparse(packet.get());
      output_buffer.push_front(std::move(packet));
    }
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

  // should this be done by the call to init_from_command_line_options
  int thrift_port = simple_switch->get_runtime_port();
  bm_runtime::start_server(simple_switch, thrift_port);

  simple_switch->start_and_return();

  while(1) std::this_thread::sleep_for(std::chrono::seconds(100));
  
  return 0; 
}
