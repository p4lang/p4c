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
    @name("ingress.tmp") bit<16> tmp;
    @hidden action issue22051bmv2l35() {
        h.eth_hdr.src_addr = 48w1;
    }
    @hidden action issue22051bmv2l34() {
        tmp = h.eth_hdr.eth_type;
        h.eth_hdr.eth_type = 16w182;
    }
    @hidden table tbl_issue22051bmv2l34 {
        actions = {
            issue22051bmv2l34();
        }
        const default_action = issue22051bmv2l34();
    }
    @hidden table tbl_issue22051bmv2l35 {
        actions = {
            issue22051bmv2l35();
        }
        const default_action = issue22051bmv2l35();
    }
    apply {
        tbl_issue22051bmv2l34.apply();
        if (tmp == 16w2) {
            tbl_issue22051bmv2l35.apply();
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
        pkt.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
