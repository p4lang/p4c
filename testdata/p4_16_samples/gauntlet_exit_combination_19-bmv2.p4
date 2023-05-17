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

action exit_action() {
    exit;
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

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    table simple_table_2 {
        key = {
            h.eth_hdr.src_addr: exact @name("key") ;
        }
        actions = {
            exit_action();
        }
    }

    apply {
        bool simple_eq = h.eth_hdr.eth_type == 1;
        bit<16> assign = simple_eq || simple_table_2.apply().hit ? 16w2 : 16w3;
        h.eth_hdr.eth_type = assign;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

