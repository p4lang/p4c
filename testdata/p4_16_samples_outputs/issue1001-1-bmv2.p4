#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct Headers {
}

enum bit<8> PreservedFieldList {
    Field = 8w1
}

struct Meta {
    @field_list(PreservedFieldList.Field) 
    bit<1> b;
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        m.b = m.b + 1;
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        clone_preserving_field_list(CloneType.I2E, 32w64, PreservedFieldList.Field);
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
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

