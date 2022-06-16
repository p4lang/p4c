#include <core.p4>

bit<4> func(in bit<4> l) {
    const int<6> tt = 6s1;
    return l << 6w1;
}
parser p(out bit<4> result) {
    state start {
        result = func(4w1);
        transition accept;
    }
}

parser P(out bit<4> r);
package top(P p);
top(p()) main;

