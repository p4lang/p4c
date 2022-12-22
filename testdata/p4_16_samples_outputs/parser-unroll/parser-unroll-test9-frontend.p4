#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_stack {
    bit<8> i1;
    bit<8> i2;
}

header h_index {
    bit<8> index;
    bit<8> counter;
}

struct headers {
    h_stack[2] h;
    h_index    i;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<h_index>(hdr.i);
        hdr.i.counter = 8w0;
        transition start_loops;
    }
    state start_loops {
        hdr.i.counter = hdr.i.counter + 8w1;
        transition select(hdr.i.index) {
            8w0: mixed_finite_loop;
            8w1: mixed_infinite_loop;
            8w2: infinite_loop;
            8w3: finite_loop;
            default: reject;
        }
    }
    state finite_loop {
        hdr.i.counter = hdr.i.counter + 8w1;
        pkt.extract<h_stack>(hdr.h.next);
        transition select(hdr.h.last.i1) {
            8w2: accept;
            default: finite_loop;
        }
    }
    state mixed_finite_loop {
        pkt.extract<h_stack>(hdr.h.next);
        transition select(hdr.h.last.i2) {
            8w1: start_loops;
            8w2: accept;
        }
    }
    state mixed_infinite_loop {
        transition select(hdr.i.counter) {
            8w3: accept;
            default: start_loops;
        }
    }
    state infinite_loop {
        hdr.i.counter = hdr.i.counter + 8w1;
        transition select(hdr.i.counter) {
            8w3: accept;
            default: infinite_loop;
        }
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control vrfy(inout headers h, inout Meta m) {
    apply {
    }
}

control update(inout headers h, inout Meta m) {
    apply {
    }
}

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit<h_stack[2]>(h.h);
        pkt.emit<h_index>(h.i);
    }
}

V1Switch<headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
