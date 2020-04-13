#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header hdr {
    bit<16> a;
}

struct Headers {
    hdr h;
}

struct Meta {
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract<hdr>(h.h);
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
        b.emit<hdr>(h.h);
    }
}

struct tuple_0 {
    bit<16> field;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action constantincalculationbmv2l27() {
        hash<bit<16>, bit<10>, tuple_0, bit<10>>(h.h.a, HashAlgorithm.crc16, 10w0, { 16w1 }, 10w4);
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_constantincalculationbmv2l27 {
        actions = {
            constantincalculationbmv2l27();
        }
        const default_action = constantincalculationbmv2l27();
    }
    apply {
        tbl_constantincalculationbmv2l27.apply();
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

