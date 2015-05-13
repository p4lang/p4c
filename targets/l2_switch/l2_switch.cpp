#include <iostream>
#include <memory>
#include <thread>
#include <fstream>

#include "bm_sim/queue.h"
#include "bm_sim/packet.h"
#include "bm_sim/parser.h"
#include "bm_sim/P4Objects.h"
#include "bm_sim/tables.h"
#include "bm_sim/switch.h"
#include "bm_sim/event_logger.h"

#include "l2_switch.h"
#include "primitives.h"
#include "simplelog.h"

#include "bm_runtime/bm_runtime.h"

class SimpleSwitch : public Switch {
public:
  SimpleSwitch(transmit_fn_t transmit_fn)
    : input_buffer(1024), output_buffer(128), transmit_fn(transmit_fn) {}

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
  transmit_fn_t transmit_fn;
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
  Pipeline *ingress_mau = p4objects->get_pipeline("ingress");
  Pipeline *egress_mau = p4objects->get_pipeline("egress");
  Parser *parser = p4objects->get_parser("parser");
  Deparser *deparser = p4objects->get_deparser("deparser");
  PHV *phv;

  while(1) {
    std::unique_ptr<Packet> packet;
    input_buffer.pop_back(&packet);
    phv = packet->get_phv();
    SIMPLELOG << "processing packet " << packet->get_packet_id() << std::endl;

    int ingress_port = packet->get_ingress_port();
    phv->get_field("standard_metadata.ingress_port").set(ingress_port);
    
    parser->parse(packet.get());
    ingress_mau->apply(packet.get());

    int egress_port = phv->get_field("standard_metadata.egress_port").get_int();
    SIMPLELOG << "egress port is " << egress_port << std::endl;

    int learn_id = phv->get_field("intrinsic_metadata.learn_id").get_int();
    SIMPLELOG << "learn id is " << learn_id << std::endl;

    unsigned int mgid = phv->get_field("intrinsic_metadata.mgid").get_uint();
    SIMPLELOG << "mgid is " << mgid << std::endl;

    if(learn_id > 0) {
      get_learn_engine()->learn(learn_id, *packet.get());
      phv->get_field("intrinsic_metadata.learn_id").set(0);
    }

    if(egress_port == 0 && mgid == 0) {
      SIMPLELOG << "dropping packet\n";
      continue;
    }

    if(mgid != 0) {
      assert(mgid == 1);
      phv->get_field("intrinsic_metadata.mgid").set(0);
      packet_id_t copy_id = 1;
      const auto pre_out = pre->replicate({mgid});
      for(const auto &out : pre_out) {
	egress_port = out.egress_port;
	if(ingress_port == egress_port) continue; // pruning
	SIMPLELOG << "replicating packet out of port " << egress_port
		  << std::endl;
	std::unique_ptr<Packet> packet_copy(new Packet());
	*packet_copy = packet->clone(copy_id++);
	packet_copy->set_egress_port(egress_port);
	egress_mau->apply(packet_copy.get());
	deparser->deparse(packet_copy.get());
	output_buffer.push_front(std::move(packet_copy));
      }
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


int packet_accept(int port_num, const char *buffer, int len) {
  return simple_switch->receive(port_num, buffer, len);
}

void start_processing(transmit_fn_t transmit_fn) {
  simple_switch = new SimpleSwitch(transmit_fn);
  simple_switch->init_objects("l2_switch.json");

  bm_runtime::start_server(simple_switch, 9090);

  // add_test_entry();

  simple_switch->start_and_return();
}

