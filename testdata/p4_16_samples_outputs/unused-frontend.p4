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
    bit<32> tmp;
    @name("c.e") E() e_0;
    apply {
        if (s.h.isValid()) 
            s.h.data3 = 32w0;
        if (s.h.data2 == 32w0) {
            tmp = e_0.get<bit<32>>(s.h.data2);
            s.h.data1 = tmp;
        }
    }
}

control cproto<T>(inout T v);
package top(cproto<_> _c);
top(c()) main;

