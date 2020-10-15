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
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bool tmp_1;
    @name("ingress.tmp_2") bool tmp_2;
    apply {
        {
            @name("ingress.hasReturned_0") bool hasReturned = false;
            @name("ingress.retval_0") bit<8> retval;
            hasReturned = true;
            retval = 8w1;
            tmp_0 = retval;
        }
        tmp_1 = tmp_0 == 8w1;
        tmp_2 = tmp_1;
        {
            @name("ingress.hasReturned") bool hasReturned_0 = false;
            @name("ingress.retval") bit<8> retval_0;
            hasReturned_0 = true;
            retval_0 = 8w1;
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
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

