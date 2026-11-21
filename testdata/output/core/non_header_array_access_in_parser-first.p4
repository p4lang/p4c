#include <core.p4>

@command_line("--loopsUnroll") struct S {
    bit<8> f1;
    bit<8> f2;
}

header h_t {
    S[2] array;
}

struct metadata_t {
    S[2] array;
    h_t  h;
}

parser p(packet_in packet, out metadata_t metadata) {
    state start {
        transition s0;
    }
    state s0 {
        transition select(packet.lookahead<bit<8>>()) {
            8w1: s1;
            default: accept;
        }
    }
    state s1 {
        packet.extract<h_t>(metadata.h);
        metadata.array[1].f1 = metadata.h.array[0].f1;
        transition accept;
    }
}

parser simple(packet_in packet, out metadata_t metadata);
package top(simple e);
top(p()) main;
