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
    ethernet_t val1_eth_hdr;
    @name("ingress.dst") bit<48> dst;
    @name("ingress.type_1") bit<16> type_1;
    @name("ingress.c") bool c_0;
    @name("ingress.c1") bool c1_0;
    @name("ingress.simple_action") action simple_action() {
        h.eth_hdr.src_addr = (h.eth_hdr.eth_type != 16w1 ? 48w1 : h.eth_hdr.src_addr);
        val1_eth_hdr = (h.eth_hdr.eth_type != 16w1 ? h.eth_hdr : val1_eth_hdr);
        dst = (h.eth_hdr.eth_type != 16w1 ? val1_eth_hdr.dst_addr : dst);
        type_1 = (h.eth_hdr.eth_type != 16w1 ? val1_eth_hdr.eth_type : type_1);
        c_0 = (h.eth_hdr.eth_type != 16w1 ? true : c_0);
        c1_0 = (h.eth_hdr.eth_type != 16w1 ? false : c1_0);
        type_1 = type_1;
        dst = (h.eth_hdr.eth_type != 16w1 ? (c_0 ? (c1_0 ? 48w1 : dst) : dst) : dst);
        dst = (h.eth_hdr.eth_type != 16w1 ? (c_0 ? (c1_0 ? dst : 48w2) : dst) : dst);
        type_1 = (h.eth_hdr.eth_type != 16w1 ? (c_0 ? type_1 : 16w3) : type_1);
        val1_eth_hdr.dst_addr = (h.eth_hdr.eth_type != 16w1 ? dst : val1_eth_hdr.dst_addr);
        val1_eth_hdr.eth_type = (h.eth_hdr.eth_type != 16w1 ? type_1 : val1_eth_hdr.eth_type);
        val1_eth_hdr.dst_addr = (h.eth_hdr.eth_type != 16w1 ? val1_eth_hdr.dst_addr + 48w3 : val1_eth_hdr.dst_addr);
        h.eth_hdr = (h.eth_hdr.eth_type != 16w1 ? val1_eth_hdr : h.eth_hdr);
    }
    @hidden action issue2345multiple_dependencies60() {
        h.eth_hdr.src_addr = 48w2;
        h.eth_hdr.dst_addr = 48w2;
        h.eth_hdr.eth_type = 16w2;
    }
    @hidden table tbl_issue2345multiple_dependencies60 {
        actions = {
            issue2345multiple_dependencies60();
        }
        const default_action = issue2345multiple_dependencies60();
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action();
        }
        const default_action = simple_action();
    }
    apply {
        tbl_issue2345multiple_dependencies60.apply();
        tbl_simple_action.apply();
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

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<ethernet_t>(h.eth_hdr);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

