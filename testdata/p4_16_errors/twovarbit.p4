#include <core.p4>

header H {
    varbit<120> x;
    varbit<120> u;
}

parser P(packet_in p) {
    H h;
    state start {
        p.extract(h, 100);
        transition accept;
    }
}