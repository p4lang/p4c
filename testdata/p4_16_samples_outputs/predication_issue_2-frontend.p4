#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

header IDX {
    bit<8> idx;
}

struct Headers {
    ethernet_t eth_hdr;
    IDX        idx;
    H[2]       h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<IDX>(hdr.idx);
        pkt.extract<H>(hdr.h.next);
        pkt.extract<H>(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.val") bit<8> val;
    @name("ingress.check") bool check_0;
    @name("ingress.val_0") bit<8> val_2;
    @name("ingress.bound_0") bit<8> bound;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.simple_action") action simple_action() {
        check_0 = true;
        if (check_0) {
            val_2 = h.idx.idx;
            bound = 8w1;
            if (val_2 < bound) {
                tmp = val_2;
            } else {
                tmp = bound;
            }
            retval = tmp;
            val = retval;
            h.h[val].a = 8w1;
        }
    }
    apply {
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
