#include <iostream>
#include <memory>
#include <thread>
#include <fstream>

#include "behavioral_sim/queue.h"
#include "behavioral_sim/packet.h"
#include "behavioral_sim/parser.h"
#include "behavioral_sim/P4Objects.h"

#include "simple_switch.h"

using std::unique_ptr;
using std::thread;

static Queue<unique_ptr<Packet> > input_buffer(1024);
static Queue<unique_ptr<Packet> > output_buffer(1024);

static P4Objects *p4objects;

class modify_field : public ActionPrimitive<Field &, const Data &> {
  void operator ()(Field &f, const Data &d) {
    f.set(d);
  }
};

REGISTER_PRIMITIVE(modify_field);

class drop : public ActionPrimitive<> {
  void operator ()() {
    get_field("standard_metadata.egress_spec").set(0);
  }
};

REGISTER_PRIMITIVE(drop);

extern "C" {

int packet_accept(int port_num, const char *buffer, int len) {
  static int pkt_id = 0;
  input_buffer.push_front(
      unique_ptr<Packet>(
	 new Packet(port_num, pkt_id++, 0, PacketBuffer(2048, buffer, len))
      )
  );
  return 0;
}

static void pipeline_thread() {
  Pipeline *ingress_mau = p4objects->get_pipeline("ingress");
  Parser *parser = p4objects->get_parser("parser");
  Deparser *deparser = p4objects->get_deparser("deparser");
  PHV &phv = p4objects->get_phv();

  (void) ingress_mau; (void) parser; (void) deparser; (void) phv;

  while(1) {
    unique_ptr<Packet> packet;
    input_buffer.pop_back(&packet);
    std::cout<< "processing packet " << packet->get_packet_id() << std::endl;

    parser->parse(packet.get(), &phv);

    deparser->deparse(phv, packet.get());

    int egress_port = phv.get_field("standard_metadata.egress_spec").get_int();
    std::cout << "egress port is " << egress_port << std::endl;
    egress_port = 4;
    packet->set_egress_port(egress_port);

    output_buffer.push_front(std::move(packet));
  }
}

static transmit_fn_t transmit_fn_;

static void transmit_thread() {
  while(1) {
    unique_ptr<Packet> packet;
    output_buffer.pop_back(&packet);
    std::cout<< "transmitting packet " << packet->get_packet_id() << std::endl;

    transmit_fn_(packet->get_egress_port(),
		 packet->data(), packet->get_data_size());
  }
}

void start_processing(transmit_fn_t transmit_fn) {
  transmit_fn_ = transmit_fn;

  p4objects = new P4Objects();
  std::fstream fs("simple_switch.json");
  p4objects->init_objects(fs);

  std::cout << "Processing packets\n";

  thread t1(pipeline_thread);
  t1.detach();

  thread t2(transmit_thread);
  t2.detach();
}

}
