#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

bit<3> max(in bit<3> val, in bit<3> bound) {
    return (val < bound ? val : bound);
}
header h_t {
}

struct Headers {
    h_t[1] hdr_arr;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action a(inout h_t hdr) {
    }
    table t {
        actions = {
            a(h.hdr_arr[max(3w0, 3w0)]);
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
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
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
