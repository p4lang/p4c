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
    bit<16> tmp;
    bit<16> tmp_0;
    apply {
        {
            bit<48> val_0 = h.eth_hdr.dst_addr;
            bool hasReturned = false;
            bit<16> retval;
            val_0 = 48w1;
            hasReturned = true;
            retval = 16w1;
            h.eth_hdr.dst_addr = val_0;
            tmp = retval;
        }
        tmp_0 = tmp[0:0] ++ 15w0;
        h.eth_hdr.eth_type = tmp_0;
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

