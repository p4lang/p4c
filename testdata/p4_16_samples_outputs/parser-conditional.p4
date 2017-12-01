#include <core.p4>

parser p(out bit<32> b) {
    bit<32> a = 1;
    state start {
        b = (a == 0 ? 32w2 : 3);
        b = b + 1;
        b = (a > 0 ? (a > 1 ? b + 1 : b + 2) : b + 3);
        transition accept;
    }
}

parser proto(out bit<32> b);
package top(proto _p);
top(p()) main;

