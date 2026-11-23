#include <core.p4>

parser p(packet_in packet) {
    value_set<bit<16>>(1) vs;
    state start {
        transition select(vs) {
            1: state1;
            default: accept;
        }
    }
    state state1 {
        transition accept;
    }
}

parser simple(packet_in packet);
package top(simple e);
top(p()) main;
