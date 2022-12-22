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
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state finite_loop {
        hdr.i.counter = hdr.i.counter + 8w1;
        pkt.extract<h_stack>(hdr.h[32w0]);
        transition select(hdr.h[32w0].i1) {
            8w2: accept;
            default: finite_loop1;
        }
    }
    state finite_loop1 {
        hdr.i.counter = hdr.i.counter + 8w1;
        pkt.extract<h_stack>(hdr.h[32w1]);
        transition select(hdr.h[32w1].i1) {
            8w2: accept;
            default: finite_loop2;
        }
    }
    state finite_loop2 {
        hdr.i.counter = hdr.i.counter + 8w1;
        transition stateOutOfBound;
    }
    state infinite_loop {
        hdr.i.counter = hdr.i.counter + 8w1;
        transition select(hdr.i.counter) {
            8w3: accept;
            default: infinite_loop;
        }
    }
    state mixed_finite_loop {
        pkt.extract<h_stack>(hdr.h[32w0]);
        transition select(hdr.h[32w0].i2) {
            8w1: start_loops1;
            8w2: accept;
            default: noMatch;
        }
    }
    state mixed_finite_loop1 {
        pkt.extract<h_stack>(hdr.h[32w1]);
        transition select(hdr.h[32w1].i2) {
            8w1: start_loops2;
            8w2: accept;
            default: noMatch;
        }
    }
    state mixed_finite_loop2 {
        transition stateOutOfBound;
    }
    state mixed_infinite_loop {
        transition select(hdr.i.counter) {
            8w3: accept;
            default: start_loops;
        }
    }
    state mixed_infinite_loop1 {
        transition select(hdr.i.counter) {
            8w3: accept;
            default: start_loops1;
        }
    }
    state mixed_infinite_loop2 {
        transition select(hdr.i.counter) {
            8w3: accept;
            default: start_loops2;
        }
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
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
    state start_loops1 {
        hdr.i.counter = hdr.i.counter + 8w1;
        transition select(hdr.i.index) {
            8w0: mixed_finite_loop1;
            8w1: mixed_infinite_loop1;
            8w2: infinite_loop;
            8w3: finite_loop1;
            default: reject;
        }
    }
    state start_loops2 {
        hdr.i.counter = hdr.i.counter + 8w1;
        transition select(hdr.i.index) {
            8w0: mixed_finite_loop2;
            8w1: mixed_infinite_loop2;
            8w2: infinite_loop;
            8w3: finite_loop2;
            default: reject;
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
        pkt.emit<h_stack>(h.h[0]);
        pkt.emit<h_stack>(h.h[1]);
        pkt.emit<h_index>(h.i);
    }
}

V1Switch<headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
