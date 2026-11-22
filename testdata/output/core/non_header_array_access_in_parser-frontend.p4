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
    @name("p.tmp") bit<8> tmp;
    @name("p.tmp_0") bit<8> tmp_0;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        tmp = tmp_0;
        transition select(tmp) {
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
