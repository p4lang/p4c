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
    @name("ingress.val_0") bit<32> val;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.action_1") action action_1() {
    }
    @name("ingress.action_1") action action_2() {
    }
    @name("ingress.action_2") action action_4() {
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.src_addr: exact @name("aiiIgQ");
        }
        actions = {
            action_1();
            action_4();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        switch (simple_table_0.apply().action_run) {
            action_1: {
                action_2();
            }
            action_4: {
            }
            default: {
            }
        }
        sm.instance_type = val;
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
