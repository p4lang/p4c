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
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        transition accept;
    }
}

bit<48> set(inout bit<48> s) {
    s = 48w1;
    return 48w2;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    Headers tmp = (Headers){eth_hdr = (ethernet_t){dst_addr = h.eth_hdr.dst_addr,src_addr = set(h.eth_hdr.dst_addr),eth_type = 16w1}};
    tuple<bit<48>, bit<48>> t = { h.eth_hdr.dst_addr, set(h.eth_hdr.dst_addr) };
    apply {
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
