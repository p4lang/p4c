#include <core.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<8>  c;
}

control ingress(inout hdr h) {
    @hidden action act() {
        h.a[7:0] = ((bit<32>)h.c)[7:0];
        h.a[15:8] = (h.c + h.c)[7:0];
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control c(inout hdr h);
package top(c _c);
top(ingress()) main;

