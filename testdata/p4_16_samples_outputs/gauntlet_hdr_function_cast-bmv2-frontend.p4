#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

struct Headers {
    ethernet_t eth_hdr1;
    ethernet_t eth_hdr2;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr1);
        pkt.extract<ethernet_t>(hdr.eth_hdr2);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.val_0") bit<16> val;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") ethernet_t retval;
    @name("ingress.retval_0") ethernet_t retval_0;
    apply {
        val = h.eth_hdr1.eth_type;
        hasReturned = false;
        if (val == 16w1) {
            hasReturned = true;
            retval.setValid();
            retval = (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
        } else if (val == 16w2) {
            hasReturned = true;
            retval.setValid();
            retval = (ethernet_t){dst_addr = 48w2,src_addr = 48w2,eth_type = 16w2};
        }
        if (hasReturned) {
            ;
        } else {
            retval.setValid();
            retval = (ethernet_t){dst_addr = 48w3,src_addr = 48w3,eth_type = 16w3};
        }
        h.eth_hdr1 = retval;
        retval_0.setValid();
        retval_0 = (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
        h.eth_hdr2 = retval_0;
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
