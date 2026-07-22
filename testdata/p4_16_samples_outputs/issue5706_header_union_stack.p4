#include <core.p4>


@command_line("--loopsUnroll") header h1 {
    bit<8> a;
}

header h2 {
    bit<16> b;
}

header_union HU {
    h1 x;
    h2 y;
}

struct headers_t {
    HU[4] hus;
}

control C(inout headers_t hdrs);
package top(C c);
control c(inout headers_t hdrs) {
    apply {
        hdrs.hus.push_front(1);
        hdrs.hus.pop_front(2);
    }
}

top(c()) main;
