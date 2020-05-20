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
    bool tmp_0;
    bool val2;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.do_action") action do_action() {
        val2 = tmp_0;
        tmp_0 = val2;
    }
    @name("ingress.simple_table") table simple_table_0 {
        key = {
        }
        actions = {
            do_action();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action issue2375bmv2l30() {
        tmp_0 = false;
    }
    @hidden table tbl_issue2375bmv2l30 {
        actions = {
            issue2375bmv2l30();
        }
        const default_action = issue2375bmv2l30();
    }
    apply {
        tbl_issue2375bmv2l30.apply();
        simple_table_0.apply();
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
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

