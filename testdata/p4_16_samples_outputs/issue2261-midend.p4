#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct nested_struct {
    ethernet_t eth_hdr;
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
    ethernet_t tmp_struct_0_eth_hdr;
    @hidden action issue2261l22() {
        tmp_struct_0_eth_hdr.setValid();
        tmp_struct_0_eth_hdr.dst_addr = 48w0;
        tmp_struct_0_eth_hdr.src_addr = 48w0;
        tmp_struct_0_eth_hdr.eth_type = 16w0;
        h.eth_hdr.eth_type = 16w0;
    }
    @hidden table tbl_issue2261l22 {
        actions = {
            issue2261l22();
        }
        const default_action = issue2261l22();
    }
    apply {
        tbl_issue2261l22.apply();
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
