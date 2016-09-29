#include <core.p4>

header H {
    bit<32> f;
}

control c() {
    packet_in() p;
    H h;
    apply {
        p.extract(h);
    }
}

control proto();
package top(proto _p);
top(c()) main;
