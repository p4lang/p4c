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
    @name("ingress.simple_action") action simple_action() {
        @name("ingress.hasReturned") bool hasReturned = false;
        if (h.eth_hdr.eth_type == 16w1) {
            hasReturned = true;
        }
        if (!hasReturned) {
            h.eth_hdr.src_addr = 48w1;
            {
                @name("ingress.val_0") Headers val_0 = h;
                {
                    @name("ingress.val1_0") Headers val1_0 = val_0;
                    val1_0.eth_hdr.dst_addr = val1_0.eth_hdr.dst_addr + 48w3;
                    val_0 = val1_0;
                }
                val_0.eth_hdr.eth_type = 16w2;
                {
                    @name("ingress.val1_1") Headers val1_1 = val_0;
                    val1_1.eth_hdr.dst_addr = val1_1.eth_hdr.dst_addr + 48w3;
                    val_0 = val1_1;
                }
                h = val_0;
            }
            h.eth_hdr.dst_addr = h.eth_hdr.dst_addr + 48w4;
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

