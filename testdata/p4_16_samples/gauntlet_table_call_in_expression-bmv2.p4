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

bit<16> simple_action() {
    return 16w1;
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

    action simple_action() {
        h.eth_hdr.dst_addr = 2;
    }
    action exit_action() {
        exit;
    }
    table simple_table {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
            simple_action;
            exit_action;
        }
    }
    apply {
        if(simple_table.apply().hit && h.eth_hdr.src_addr == h.eth_hdr.dst_addr) {
            h.eth_hdr.src_addr = 2;
        }
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

