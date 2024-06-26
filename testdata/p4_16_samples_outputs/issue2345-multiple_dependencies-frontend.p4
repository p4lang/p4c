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
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.val1") Headers val1;
    @name("ingress.dst") bit<48> dst;
    @name("ingress.type") bit<16> type_1;
    @name("ingress.c") bool c_0;
    @name("ingress.c1") bool c1_0;
    @name("ingress.simple_action") action simple_action() {
        hasReturned = false;
        if (h.eth_hdr.eth_type == 16w1) {
            hasReturned = true;
        }
        if (hasReturned) {
            ;
        } else {
            h.eth_hdr.src_addr = 48w1;
            val1 = h;
            dst = val1.eth_hdr.dst_addr;
            type_1 = val1.eth_hdr.eth_type;
            c_0 = true;
            c1_0 = false;
            if (c_0) {
                type_1 = type_1;
                if (c1_0) {
                    dst = 48w1;
                } else {
                    dst = 48w2;
                }
            } else {
                type_1 = 16w3;
            }
            val1.eth_hdr.dst_addr = dst;
            val1.eth_hdr.eth_type = type_1;
            val1.eth_hdr.dst_addr = val1.eth_hdr.dst_addr + 48w3;
            h = val1;
        }
    }
    apply {
        h.eth_hdr.src_addr = 48w2;
        h.eth_hdr.dst_addr = 48w2;
        h.eth_hdr.eth_type = 16w2;
        simple_action();
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
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
