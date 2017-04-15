#include <core.p4>

header h_t {
    bit<8>    f;
    varbit<8> g;
}

struct my_packet {
    h_t h;
}

parser MyParser(packet_in b, out my_packet p) {
    state start {
        b.extract(p.h);
        transition accept;
    }
}

