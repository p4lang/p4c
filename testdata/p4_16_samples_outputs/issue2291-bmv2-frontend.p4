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
    @name("ingress.tmp_0") bit<16> tmp;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<16> retval;
    @name("ingress.hasReturned_0") bool hasReturned_0;
    @name("ingress.retval_0") bit<16> retval_0;
    @name("ingress.tmp") bit<16> tmp_0;
    @name("ingress.hasReturned") bool hasReturned_3;
    @name("ingress.retval") bit<16> retval_3;
    @name("ingress.hasReturned") bool hasReturned_4;
    @name("ingress.retval") bit<16> retval_4;
    @name("ingress.hasReturned") bool hasReturned_5;
    @name("ingress.retval") bit<16> retval_5;
    @name("ingress.simple_action") action simple_action() {
        hasReturned = false;
        hasReturned = true;
        retval = 16w4;
        hasReturned_0 = false;
        hasReturned_3 = false;
        hasReturned_3 = true;
        retval_3 = 16w4;
        tmp_0 = retval_3;
        hasReturned_0 = true;
        retval_0 = tmp_0;
    }
    apply {
        hasReturned_4 = false;
        hasReturned_4 = true;
        retval_4 = 16w4;
        tmp = retval_4;
        hasReturned_5 = false;
        hasReturned_5 = true;
        retval_5 = 16w4;
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

