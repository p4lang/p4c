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
    action set_valid_action(out Headers tmp) {
        tmp.eth_hdr.setValid();
        tmp.eth_hdr.dst_addr = 1;
        tmp.eth_hdr.src_addr = 1;
    }
    table simple_table_1 {
        key = {
            128w1 : exact @name("JGOUaj") ;
        }
        actions = {
            set_valid_action(h);
        }
    }
    table simple_table_2 {
        key = {
            48w1: exact @name("qkgOtm") ;
        }
        actions = {
            NoAction();
        }
    }
    apply {
        switch (simple_table_2.apply().action_run) {
            NoAction: {
                simple_table_1.apply();
            }
        }
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

