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
    bool pointless_bool_0;
    bit<48> tmp;
    @name("ingress.do_action") action do_action() {
        bool hasReturned = false;
        pointless_bool_0 = true;
        if (h.eth_hdr.dst_addr != 48w0) {
            ;
        } else {
            hasReturned = true;
        }
        if (!hasReturned) {
            {
                bit<16> val1_0 = h.eth_hdr.eth_type;
                bit<48> val2_0 = h.eth_hdr.src_addr;
                h.eth_hdr.eth_type = val1_0;
                h.eth_hdr.src_addr = val2_0;
            }
            if (pointless_bool_0) {
                tmp = 48w1;
            } else {
                tmp = 48w2;
            }
            h.eth_hdr.src_addr = tmp;
        }
    }
    apply {
        do_action();
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
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

