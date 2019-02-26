#include <core.p4>

header Header {
    bit<32>     data1;
    bit<32>     data2;
    bit<32>     data3;
    varbit<320> options;
    bit<8>      length;
}

parser p1(packet_in p, out Header h) {
    state start {
        p.extract(h, 320);
        transition accept;
    }
}

control c(out bit<32> v) {
    apply {
    }
}

parser proto(packet_in p, out Header h);
control cproto(out bit<32> v);
package top(proto _p, cproto _c);
top(p1(), c()) main;

