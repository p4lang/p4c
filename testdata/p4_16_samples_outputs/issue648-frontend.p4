#include <core.p4>

header hdr {
    bit<32> a;
    bit<32> b;
    bit<8>  c;
}

control ingress(inout hdr h) {
    apply {
        h.a[7:0] = ((bit<32>)h.c)[7:0];
        h.a[15:8] = (h.c + h.c)[7:0];
    }
}

control c(inout hdr h);
package top(c _c);
top(ingress()) main;

