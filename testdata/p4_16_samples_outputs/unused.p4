#include <core.p4>

header Header {
    bit<32> data1;
    bit<32> data2;
    bit<32> data3;
}

struct S {
    Header h;
}

extern E {
    E();
    bit<32> get<T>(in T b);
}

control c(inout S s) {
    E() e;
    apply {
        if (s.h.isValid()) {
            s.h.data3 = 0;
        }
        if (s.h.data2 == 0) {
            s.h.data1 = e.get(s.h.data2);
        }
    }
}

parser proto(packet_in p, out Header h);
control cproto<T>(inout T v);
package top(cproto<_> _c);
top(c()) main;

