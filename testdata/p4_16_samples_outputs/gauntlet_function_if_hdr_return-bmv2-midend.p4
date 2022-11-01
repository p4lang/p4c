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
    @name("ingress.retval") ethernet_t retval;
    @hidden action gauntlet_function_if_hdr_returnbmv2l21() {
        retval.setValid();
        retval.dst_addr = 48w1;
        retval.src_addr = 48w1;
        retval.eth_type = 16w1;
        h.eth_hdr = retval;
    }
    @hidden table tbl_gauntlet_function_if_hdr_returnbmv2l21 {
        actions = {
            gauntlet_function_if_hdr_returnbmv2l21();
        }
        const default_action = gauntlet_function_if_hdr_returnbmv2l21();
    }
    apply {
        tbl_gauntlet_function_if_hdr_returnbmv2l21.apply();
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
