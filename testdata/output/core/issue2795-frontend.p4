#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

control c(packet_out p) {
    @name("c.tmp") H tmp;
    @name("c.tmp_0") H tmp_0;
    apply {
        tmp.setValid();
        tmp = (H){a = 32w0,b = 32w1};
        p.emit<H>(tmp);
        tmp_0.setValid();
        tmp_0 = (H){a = 32w0,b = 32w1};
        p.emit<H>(tmp_0);
    }
}

control ctr(packet_out p);
package top(ctr _c);
top(c()) main;
