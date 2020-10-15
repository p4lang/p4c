#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<16> a;
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
    @name("ingress.tmp") Headers tmp;
    @name("ingress.tmp") ethernet_t tmp_0;
    @name("ingress.tmp_1") bit<48> tmp_1;
    @name("ingress.tmp_2") bit<48> tmp_2;
    @name("ingress.tmp_3") bit<48> tmp_3;
    @name("ingress.tmp_4") bit<16> tmp_4;
    @name("ingress.tmp_5") bit<48> tmp_5;
    @name("ingress.tmp_6") bit<48> tmp_6;
    @name("ingress.tmp_7") bit<48> tmp_7;
    apply {
        tmp_1 = h.eth_hdr.dst_addr;
        {
            @name("ingress.s_0") bit<48> s_0 = h.eth_hdr.dst_addr;
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<48> retval;
            s_0 = 48w1;
            hasReturned = true;
            retval = 48w2;
            h.eth_hdr.dst_addr = s_0;
            tmp_3 = retval;
        }
        tmp_2 = tmp_3;
        tmp_4 = 16w1;
        tmp_0.setValid();
        tmp_0 = (ethernet_t){dst_addr = tmp_1,src_addr = tmp_2,eth_type = tmp_4};
        tmp = (Headers){eth_hdr = tmp_0};
        tmp_5 = h.eth_hdr.dst_addr;
        {
            @name("ingress.s_1") bit<48> s_1 = h.eth_hdr.dst_addr;
            @name("ingress.hasReturned") bool hasReturned_1 = false;
            @name("ingress.retval") bit<48> retval_1;
            s_1 = 48w1;
            hasReturned_1 = true;
            retval_1 = 48w2;
            h.eth_hdr.dst_addr = s_1;
            tmp_7 = retval_1;
        }
        tmp_6 = tmp_7;
        h = tmp;
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

