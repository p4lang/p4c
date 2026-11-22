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

ethernet_t do_function_1(inout bit<16> val) {
    if (val == 16w1) {
        return (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
    } else if (val == 16w2) {
        return (ethernet_t){dst_addr = 48w2,src_addr = 48w2,eth_type = 16w2};
    }
    return (ethernet_t){dst_addr = 48w3,src_addr = 48w3,eth_type = 16w3};
}
ethernet_t do_function_2() {
    return (ethernet_t){dst_addr = 48w1,src_addr = 48w1,eth_type = 16w1};
}
parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract<ethernet_t>(hdr.eth_hdr1);
        pkt.extract<ethernet_t>(hdr.eth_hdr2);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.eth_hdr1 = do_function_1(h.eth_hdr1.eth_type);
        h.eth_hdr2 = do_function_2();
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
