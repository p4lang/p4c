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
    @name("ingress.simple_eq") bool simple_eq_0;
    @name("ingress.assign") bit<16> assign_0;
    @name("ingress.tmp") bool tmp;
    @name("ingress.tmp_0") bool tmp_0;
    @name("ingress.tmp_1") bit<16> tmp_1;
    @name(".exit_action") action exit_action_0() {
        exit;
    }
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_table_2") table simple_table {
        key = {
            h.eth_hdr.src_addr: exact @name("key");
        }
        actions = {
            exit_action_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        simple_eq_0 = h.eth_hdr.eth_type == 16w1;
        if (simple_eq_0) {
            tmp = true;
        } else {
            tmp_0 = simple_table.apply().hit;
            tmp = tmp_0;
        }
        if (tmp) {
            tmp_1 = 16w2;
        } else {
            tmp_1 = 16w3;
        }
        assign_0 = tmp_1;
        h.eth_hdr.eth_type = assign_0;
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
