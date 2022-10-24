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
    @name("ingress.tmp") bool tmp;
    @name("ingress.tmp_0") bit<16> tmp_0;
    @name("ingress.tmp_1") bit<16> tmp_1;
    @name("ingress.input_value_0") bit<16> input_value;
    @name("ingress.retval") bit<16> retval;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.exit_action") action exit_action() {
        exit;
    }
    @name("ingress.exit_table") table exit_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("key");
        }
        actions = {
            exit_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp = exit_table_0.apply().hit;
        if (tmp) {
            tmp_0 = 16w1;
        } else {
            tmp_0 = 16w2;
        }
        tmp_1 = tmp_0;
        input_value = tmp_1;
        retval = input_value;
        h.eth_hdr.eth_type = retval;
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
