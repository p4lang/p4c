#include <core.p4>

extern void f(in bit<32> x);

parser p0(packet_in p) {
    state start {
        f(_);
        transition accept;
    }
}

parser proto(packet_in p);
package top(proto p);
top(p0()) main;
