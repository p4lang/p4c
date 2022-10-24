#include <core.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<8>  c;
}

control ingress(inout hdr h) {
    @hidden action issue648l11() {
        h.a[7:0] = h.c;
        h.a[15:8] = h.c + h.c;
    }
    @hidden table tbl_issue648l11 {
        actions = {
            issue648l11();
        }
        const default_action = issue648l11();
    }
    apply {
        tbl_issue648l11.apply();
    }
}

control c(inout hdr h);
package top(c _c);
top(ingress()) main;
