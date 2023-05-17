#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t    eth_hdr;
}

struct Meta {}

bit<48> do_thing(inout bit<48> val_0, out bit<48> val_1) {
    if (val_0 <= 100) {
        if (val_0 <= 50) {
            val_1 = 48w3;
            return 48w0;
        } else {
            val_1 = 48w12;
        }
        if (val_0 <= 25) {
            val_1 = 48w1452;
        }
    }
    return 48w0;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        do_thing(h.eth_hdr.src_addr, h.eth_hdr.src_addr);
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
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

