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
    ethernet_t val_0_eth_hdr;
    ethernet_t val1_0_eth_hdr;
    ethernet_t val1_1_eth_hdr;
    bool cond_0;
    @name("ingress.simple_action") action simple_action() {
        cond_0 = h.eth_hdr.eth_type != 16w1;
        h.eth_hdr.src_addr = (h.eth_hdr.eth_type != 16w1 ? 48w1 : h.eth_hdr.src_addr);
        val_0_eth_hdr = (h.eth_hdr.eth_type != 16w1 ? h.eth_hdr : val_0_eth_hdr);
        val1_0_eth_hdr = (h.eth_hdr.eth_type != 16w1 ? val_0_eth_hdr : val1_0_eth_hdr);
        val1_0_eth_hdr.dst_addr = (h.eth_hdr.eth_type != 16w1 ? val1_0_eth_hdr.dst_addr + 48w3 : val1_0_eth_hdr.dst_addr);
        val_0_eth_hdr = (h.eth_hdr.eth_type != 16w1 ? val1_0_eth_hdr : val_0_eth_hdr);
        val_0_eth_hdr.eth_type = (h.eth_hdr.eth_type != 16w1 ? 16w2 : val_0_eth_hdr.eth_type);
        val1_1_eth_hdr = (h.eth_hdr.eth_type != 16w1 ? val_0_eth_hdr : val1_1_eth_hdr);
        val1_1_eth_hdr.dst_addr = (h.eth_hdr.eth_type != 16w1 ? val1_1_eth_hdr.dst_addr + 48w3 : val1_1_eth_hdr.dst_addr);
        val_0_eth_hdr = (h.eth_hdr.eth_type != 16w1 ? val1_1_eth_hdr : val_0_eth_hdr);
        h.eth_hdr = (h.eth_hdr.eth_type != 16w1 ? val_0_eth_hdr : h.eth_hdr);
        h.eth_hdr.dst_addr = (cond_0 ? h.eth_hdr.dst_addr + 48w4 : h.eth_hdr.dst_addr);
    }
    @hidden action issue23452l49() {
        h.eth_hdr.src_addr = 48w2;
        h.eth_hdr.dst_addr = 48w2;
        h.eth_hdr.eth_type = 16w2;
    }
    @hidden table tbl_issue23452l49 {
        actions = {
            issue23452l49();
        }
        const default_action = issue23452l49();
    }
    @hidden table tbl_simple_action {
        actions = {
            simple_action();
        }
        const default_action = simple_action();
    }
    apply {
        tbl_issue23452l49.apply();
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

