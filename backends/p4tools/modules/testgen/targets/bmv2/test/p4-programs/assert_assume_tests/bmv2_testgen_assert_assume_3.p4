#include <v1model.p4>
#include <testgen.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
    bit<16> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
  apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply {
        // The packet should be exactly 144 bits, which is h.eth_hdr + h.h
        testgen_assume(s.packet_length == 18);
        // We can not fail this assertion because we assume the packet is just long enough.
        testgen_assert(h.eth_hdr.isValid());
        // Assume that all packets ether types values are >= 0xFFFF.
        testgen_assume(h.eth_hdr.eth_type == 0xFFFF);
        // There is only one possible eth_type counterexample, which is 0xFFFF.
        // Also only one packet, since we constrain the packet length.
        testgen_assert(h.eth_hdr.eth_type != 0xFFFF);
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
