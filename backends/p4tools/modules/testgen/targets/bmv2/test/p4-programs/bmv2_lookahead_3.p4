#include <v1model.p4>

header h_t {
  bit<32> f1;
  bit<32> f2;
}

struct headers_t {
  h_t h;
}

struct user_metadata_t {
}

parser InParser(
    packet_in pkt,
    out headers_t hdr,
    inout user_metadata_t md,
    inout standard_metadata_t ig_intr_md) {

  state start {
    pkt.extract(hdr.h);
    transition h;
  }

  state h {
    hdr.h.setValid();
    hdr.h = pkt.lookahead<h_t>();
    pkt.advance(40);
    transition accept;
  }
}

control SwitchIngress(
    inout headers_t hdr,
    inout user_metadata_t md,
    inout standard_metadata_t ig_intr_md) {

  apply {
  }
}


control SwitchEgress(
    inout headers_t hdr,
    inout user_metadata_t md,
    inout standard_metadata_t ig_intr_md) {

  apply {} 
}


control vrfy(inout headers_t h, inout user_metadata_t meta) {
  apply { }
}

control update(inout headers_t h, inout user_metadata_t m) {
    apply { }
}

control deparser(packet_out pkt, in headers_t h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(InParser(), vrfy(), SwitchIngress(), SwitchEgress(), update(), deparser()) main;