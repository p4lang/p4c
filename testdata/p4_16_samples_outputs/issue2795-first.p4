#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

control c(packet_out p) {
    apply {
        p.emit<H>((H){a = 32w0,b = 32w1});
        p.emit<H>((H){a = 32w0,b = 32w1});
    }
}

control ctr(packet_out p);
package top(ctr _c);
top(c()) main;
