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

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<8> val;
    bit<8> tmp;
    bit<8> tmp_0;
    bit<8> tmp_1;
    apply {
        val = 8w3;
        tmp = val;
        {
            bit<8> some_input_0 = val;
            bool hasReturned = false;
            bit<8> retval;
            some_input_0 = 8w1;
            hasReturned = true;
            retval = 8w1;
            val = some_input_0;
            tmp_0 = retval;
        }
        tmp_1 = tmp + tmp_0;
        h.h.a = tmp_1;
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta meta, inout standard_metadata_t stdmeta) {
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

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

