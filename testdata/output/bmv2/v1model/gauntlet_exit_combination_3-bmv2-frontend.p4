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
    @name("ingress.val") bit<48> val_0;
    @name("ingress.val") bit<48> val_2;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.do_action") action do_action() {
        val_0 = 48w2;
        h.eth_hdr.src_addr = val_0;
        exit;
    }
    @name("ingress.do_action") action do_action_1() {
        val_2 = 48w2;
        h.eth_hdr.dst_addr = val_2;
        exit;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("key");
        }
        actions = {
            do_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        hasReturned = false;
        switch (simple_table_0.apply().action_run) {
            do_action: {
                hasReturned = true;
            }
            default: {
            }
        }
        if (hasReturned) {
            ;
        } else {
            do_action_1();
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
