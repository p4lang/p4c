#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<8> tmp_val = 1;
    action simple_action(inout bit<4> dummy) {
    }
    table simple_table {
        key = {
            tmp_val : exact @name("dummy") ;
        }
        actions = {
            NoAction();
        }
    }
    apply {
        simple_action(tmp_val[7:4]);
        if (simple_table.apply().hit) {
            h.eth_hdr.eth_type = 1;
        }
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h.eth_hdr);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
