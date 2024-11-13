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
        pkt.extract(hdr.eth_hdr);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<8> ret = 0;
    action sub_a1(in bit<16> x) {
        if (h.eth_hdr.eth_type >= x + 10) {
            ret = ret + 1;
            return;
        } else if (h.eth_hdr.eth_type >= x + 5) {
            ret = ret + 2;
        } else if (h.eth_hdr.eth_type == x) {
            ret = ret + 4;
            exit;
        }
        ret = ret + 8;
        h.eth_hdr.src_addr[7:0] = ret;
    }
    action top_a1(bit<16> y) {
        sub_a1(y);
        if (h.eth_hdr.src_addr == h.eth_hdr.dst_addr) {
            ret = ret + 16;
        } else {
            ret = ret + 32;
            return;
        }
        ret = ret + 64;
    }
    table t1 {
        key = {
            h.eth_hdr.dst_addr: exact;
        }
        actions = {
            top_a1;
            NoAction;
        }
        const default_action = NoAction();
        size = 32;
    }
    apply {
        if (h.eth_hdr.isValid()) {
            t1.apply();
            h.eth_hdr.src_addr[7:0] = ret;
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
        pkt.emit(h.eth_hdr);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
