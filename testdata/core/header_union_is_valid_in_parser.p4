#include <core.p4>
@command_line("--loopsUnroll")

header H {
    bit<32> a;
}

header_union HU {
    H h1;
    H h2;
}

parser p(in HU hu, out bool b) {
    state start {
        b = hu.isValid();
        transition accept;
    }
}

parser proto(in HU hu, out bool b);
package top(proto p);
top(p()) main;
