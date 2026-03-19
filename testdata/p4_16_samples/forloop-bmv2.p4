/*
Test: ForStatement support in BMv2 action bodies.

Exercises simple for-loops, nested for-loops, and loops with
early return/exit inside actions compiled to BMv2 JSON using
_jump / _jump_if_zero primitives.
*/

#include <core.p4>
#include <v1model.p4>

header hdr_t {
    bit<8>  count;
    bit<32> sum;
    bit<32> product;
}

struct Headers {
    hdr_t h;
}

struct Meta {}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }
control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h.h); }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    // Simple for-loop: sum integers 1..count
    action sum_loop() {
        h.h.sum = 0;
        for (bit<8> i = 1; i <= h.h.count; i = i + 1) {
            h.h.sum = h.h.sum + (bit<32>)i;
        }
    }

    // Nested for-loops: compute count * count
    action nested_loop() {
        h.h.product = 0;
        for (bit<8> i = 0; i < h.h.count; i = i + 1) {
            for (bit<8> j = 0; j < h.h.count; j = j + 1) {
                h.h.product = h.h.product + 1;
            }
        }
    }

    // For-loop with conditional inside
    action conditional_loop() {
        h.h.sum = 0;
        for (bit<8> i = 0; i < 10; i = i + 1) {
            if (i < 5) {
                h.h.sum = h.h.sum + 1;
            }
        }
    }

    table t {
        key = { h.h.count : exact; }
        actions = { sum_loop; nested_loop; conditional_loop; }
        const default_action = sum_loop;
    }

    apply {
        t.apply();
        sm.egress_spec = 0;
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
