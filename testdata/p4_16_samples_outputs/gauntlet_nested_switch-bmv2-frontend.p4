#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.simple_val") bit<16> simple_val_0;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.val") bit<48> val_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.call_action") action call_action() {
        val_0 = h.eth_hdr.src_addr;
        h.eth_hdr.src_addr = val_0;
    }
    @name("ingress.simple_table") table simple_table_0 {
        actions = {
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        hasReturned = false;
        simple_val_0 = 16w2;
        call_action();
        if (simple_val_0 <= 16w5) {
            hasReturned = true;
        } else {
            switch (simple_table_0.apply().action_run) {
                default: {
                    h.eth_hdr.dst_addr = 48w32;
                }
            }
        }
        if (hasReturned) {
            ;
        } else {
            h.eth_hdr.eth_type = 16w1;
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

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
