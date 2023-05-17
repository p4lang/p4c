#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

typedef bit<32> dummy_def;
const dummy_def prefix = 2348810240;

struct Headers {
    ethernet_t eth_hdr;
    H h;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.eth_hdr);
        transition select(hdr.eth_hdr.dst_addr[31:32 -1]) {
            prefix[31:32 - 1]: do_parse;
            default: accept;
        }
    }
    state do_parse {
        pkt.extract(hdr.h);
        hdr.eth_hdr.dst_addr = 1;
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

