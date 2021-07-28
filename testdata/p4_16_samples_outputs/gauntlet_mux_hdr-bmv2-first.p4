#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

bit<32> simple_function() {
    H[2] tmp1;
    H[2] tmp2;
    if (tmp2[0].a <= 32w3) {
        tmp1[0] = tmp2[1];
        tmp2[1] = tmp1[1];
    }
    return tmp1[0].a;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action simple_action() {
        h.h.a = simple_function();
    }
    table simple_table {
        key = {
            sm.egress_spec: exact @name("key") ;
        }
        actions = {
            simple_action();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        simple_table.apply();
    }
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
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

