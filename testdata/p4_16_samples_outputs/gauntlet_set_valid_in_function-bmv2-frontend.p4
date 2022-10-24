#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8>  a;
    bit<64> b;
    bit<16> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
    bit<32> t;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.retval") bit<16> retval;
    @name("ingress.tmp_ret") H tmp_ret_0;
    apply {
        tmp_ret_0.setValid();
        tmp_ret_0 = (H){a = 8w0,b = 64w0,c = 16w0};
        retval = tmp_ret_0.c;
        tmp = retval;
        m.t = (bit<32>)tmp;
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
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
