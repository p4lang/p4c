#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct bool_struct {
    bool   is_bool;
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
    bool_struct tmp = {false};

    action dummy_action(out bit<16> dummy_bit, out bool_struct dummy_struct) {
    }
    table simple_table {
        key = {
            h.eth_hdr.src_addr: exact @name("MsRuxx") ;
        }
        actions = {
            dummy_action(h.eth_hdr.eth_type, tmp);
        }
    }
    apply {
        switch (simple_table.apply().action_run) {
            dummy_action: {
                if (!tmp.is_bool) {
                    exit;
                }
                h.eth_hdr.dst_addr = 1;

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

