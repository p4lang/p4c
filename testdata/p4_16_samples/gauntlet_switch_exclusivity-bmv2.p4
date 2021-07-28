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
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action action_0() {}
    action action_1() {}
    table simple_table {
        key = {
            h.eth_hdr.src_addr: exact;
        }
        actions = {
            action_0();
            action_1();
        }
    }
    apply {
        bit<48> val_0 = 48w20;
        switch (simple_table.apply().action_run) {
            action_0: {
                h.eth_hdr.src_addr = val_0;
            }
            action_1: {
                val_0 = 48w10;
            }
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

