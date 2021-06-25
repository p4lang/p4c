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
    @name("ingress.tmp_0") bool tmp_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.dst_addr = 48w2;
    }
    @name("ingress.exit_action") action exit_action() {
        exit;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
            h.eth_hdr.eth_type: exact @name("dummy_name") ;
        }
        actions = {
            simple_action();
            exit_action();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp = simple_table_0.apply().hit;
        if (tmp) {
            tmp_0 = h.eth_hdr.src_addr == h.eth_hdr.dst_addr;
        } else {
            tmp_0 = false;
        }
        if (tmp_0) {
            h.eth_hdr.src_addr = 48w2;
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

