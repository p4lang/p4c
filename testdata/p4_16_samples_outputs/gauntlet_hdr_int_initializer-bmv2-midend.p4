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
    @name("ingress.tmp_hdr") ethernet_t tmp_hdr_0;
    @hidden action gauntlet_hdr_int_initializerbmv2l29() {
        tmp_hdr_0.setValid();
        tmp_hdr_0.dst_addr = 48w1;
        tmp_hdr_0.src_addr = 48w1;
        tmp_hdr_0.eth_type = 16w1;
        h.eth_hdr.eth_type = 16w0;
    }
    @hidden table tbl_gauntlet_hdr_int_initializerbmv2l29 {
        actions = {
            gauntlet_hdr_int_initializerbmv2l29();
        }
        const default_action = gauntlet_hdr_int_initializerbmv2l29();
    }
    apply {
        tbl_gauntlet_hdr_int_initializerbmv2l29.apply();
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
