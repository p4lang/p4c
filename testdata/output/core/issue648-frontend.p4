#include <core.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<8>  c;
}

control ingress(inout hdr h) {
    apply {
        h.a[7:0] = h.c;
        h.a[15:8] = h.c + h.c;
    }
}

control c(inout hdr h);
package top(c _c);
top(ingress()) main;
