#include <core.p4>

header H {
    bit<32> a;
    bit<32> b;
}

control c(packet_out p) {
    @name("c.tmp") H tmp;
    @hidden action issue2795l10() {
        tmp.setValid();
        tmp.a = 32w0;
        tmp.b = 32w1;
        p.emit<H>(tmp);
    }
    @hidden table tbl_issue2795l10 {
        actions = {
            issue2795l10();
        }
        const default_action = issue2795l10();
    }
    apply {
        tbl_issue2795l10.apply();
    }
}

control ctr(packet_out p);
package top(ctr _c);
top(c()) main;

