#include <core.p4>

@command_line("--loopsUnroll")

struct S {
    bit<8> f1;
    bit<8> f2;
}

header h_t {
    S[2] array;
}

struct metadata_t {
    S[2] array;
    bit<8>[2] scalar_array;
    h_t h;
}

parser p(packet_in packet, out metadata_t metadata) {
    state start {
        bit<8> idx = packet.lookahead<bit<8>>();
        // Dynamic index access in parser on struct array and scalar array
        metadata.array[idx].f1 = 8w1;
        bit<8> val = metadata.array[idx].f2;
        metadata.scalar_array[idx] = val;
        transition accept;
    }
}

parser simple(packet_in packet, out metadata_t metadata);
package top(simple e);
top(p()) main;
