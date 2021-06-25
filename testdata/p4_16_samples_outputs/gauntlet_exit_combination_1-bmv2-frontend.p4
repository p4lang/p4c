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
    @name("ingress.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.dummy") action dummy() {
    }
    @name("ingress.do_action") action do_action() {
        h.eth_hdr.eth_type = 16w1;
        exit;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            32w1: exact @name("akSTMF") ;
        }
        actions = {
            dummy();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        hasReturned = false;
        switch (simple_table_0.apply().action_run) {
            dummy: {
            }
            default: {
                hasReturned = true;
            }
        }
        if (hasReturned) {
            ;
        } else {
            do_action();
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
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

