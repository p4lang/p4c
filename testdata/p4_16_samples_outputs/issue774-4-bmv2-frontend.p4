#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header Header {
    bit<32> data;
}

struct Headers {
    Header h;
}

struct M {
}

parser prs(packet_in p, out Headers h, inout M meta, inout standard_metadata_t s) {
    Header arg;
    state start {
        p.extract<Header>(arg);
        p.extract<Header>(h.h);
        transition accept;
    }
}

control vc(inout Headers hdr, inout M meta) {
    apply {
    }
}

control i(inout Headers hdr, inout M meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control e(inout Headers hdr, inout M meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control cc(inout Headers hdr, inout M meta) {
    apply {
    }
}

control d(packet_out b, in Headers hdr) {
    apply {
        b.emit<Headers>(hdr);
    }
}

V1Switch<Headers, M>(prs(), vc(), i(), e(), cc(), d()) main;

