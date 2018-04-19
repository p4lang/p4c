#include <core.p4>

header H {
    bit<32> u;
}

control c() {
    apply {
        H[3] h;
        bit<32> x = 32w1;
        h.push_front(x);
    }
}

control proto();
package top(proto _p);
top(c()) main;

