#include <core.p4>

header H {
    varbit<120> x;
}

parser p(packet_in pkt) {
    H h;
    state start {
        h = pkt.lookahead<H>();
        transition accept;
    }
}

