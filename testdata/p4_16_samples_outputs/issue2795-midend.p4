#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

control c(packet_out p) {
    @name("c.tmp") H tmp;
    @name("c.tmp_0") H tmp_0;
    @hidden action issue2795l16() {
        tmp.setValid();
        tmp.a = 32w0;
        tmp.b = 32w1;
        p.emit<H>(tmp);
        tmp_0.setValid();
        tmp_0.a = 32w0;
        tmp_0.b = 32w1;
        p.emit<H>(tmp_0);
    }
    @hidden table tbl_issue2795l16 {
        actions = {
            issue2795l16();
        }
        const default_action = issue2795l16();
    }
    apply {
        tbl_issue2795l16.apply();
    }
}

control ctr(packet_out p);
package top(ctr _c);
top(c()) main;
