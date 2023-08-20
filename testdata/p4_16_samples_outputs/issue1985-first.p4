#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_t {
    bit<8> x;
}

struct headers {
    h_t h;
}

struct metadata {
}

parser p(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t std_meta) {
    state start {
        transition accept;
    }
}

control c1(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control c2(inout headers hdr, inout metadata meta, inout standard_metadata_t std_meta) {
    apply {
    }
}

control c3(inout headers hdr, inout metadata meta, inout standard_metadata_t std_meta) {
    action a(in bool b) {
        hdr.h.x = 8w0;
    }
    table t {
        actions = {
            a(hdr.h.isValid() || true);
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control c4(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control c5(packet_out pkt, in headers hdr) {
    apply {
    }
}

V1Switch<headers, metadata>(p(), c1(), c2(), c3(), c4(), c5()) main;
