#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

control c(packet_out p) {
    apply {
        p.emit((H){0, 1});
        p.emit<H>({0,1});
    }
}

control ctr(packet_out p);
package top(ctr _c);

top(c()) main;
