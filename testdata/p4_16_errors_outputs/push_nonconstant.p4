#include <core.p4>

header H {
    bit<32> u;
}

control c() {
    apply {
        H[3] h;
        bit<32> x = 1;
        h.push_front(x);
    }
}

