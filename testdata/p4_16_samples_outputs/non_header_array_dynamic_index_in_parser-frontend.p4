#include <core.p4>


@command_line("--loopsUnroll") struct S {
    bit<8> f1;
    bit<8> f2;
}

header h_t {
    S[2] array;
}

struct metadata_t {
    S[2]      array;
    bit<8>[2] scalar_array;
    h_t       h;
}

parser p(packet_in packet, out metadata_t metadata) {
    @name("p.idx") bit<8> idx_0;
    @name("p.val") bit<8> val_0;
    state start {
        idx_0 = packet.lookahead<bit<8>>();
        metadata.array[idx_0].f1 = 8w1;
        val_0 = metadata.array[idx_0].f2;
        metadata.scalar_array[idx_0] = val_0;
        transition accept;
    }
}

parser simple(packet_in packet, out metadata_t metadata);
package top(simple e);
top(p()) main;
