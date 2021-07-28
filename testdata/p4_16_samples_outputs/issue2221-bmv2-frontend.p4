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
    @name("ingress.tmp_0") bit<16> tmp_0;
    @name("ingress.tmp_3") bit<16> tmp_3;
    @name("ingress.tmp_6") bit<16> tmp_4;
    @name("ingress.eth_type_0") bit<16> eth_type_3;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<16> retval;
    @name("ingress.eth_type_1") bit<16> eth_type_4;
    @name("ingress.hasReturned") bool hasReturned_1;
    @name("ingress.retval") bit<16> retval_1;
    @name("ingress.eth_type_2") bit<16> eth_type_5;
    @name("ingress.hasReturned") bool hasReturned_2;
    @name("ingress.retval") bit<16> retval_2;
    apply {
        eth_type_3 = h.eth_hdr.eth_type;
        hasReturned = false;
        eth_type_3 = 16w0x806;
        hasReturned = true;
        retval = 16w2;
        h.eth_hdr.eth_type = eth_type_3;
        tmp_0 = retval;
        eth_type_4 = h.eth_hdr.eth_type;
        hasReturned_1 = false;
        eth_type_4 = 16w0x806;
        hasReturned_1 = true;
        retval_1 = 16w2;
        h.eth_hdr.eth_type = eth_type_4;
        tmp_3 = retval_1;
        eth_type_5 = h.eth_hdr.eth_type;
        hasReturned_2 = false;
        eth_type_5 = 16w0x806;
        hasReturned_2 = true;
        retval_2 = 16w2;
        h.eth_hdr.eth_type = eth_type_5;
        tmp_4 = retval_2;
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

