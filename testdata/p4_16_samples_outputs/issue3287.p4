#include <core.p4>

bit<4> func(in bit<4> l) {
    const int<6> tt = 1;
    return l << tt;
}
parser p(out bit<4> result) {
    state start {
        result = func(1);
        transition accept;
    }
}

parser P(out bit<4> r);
package top(P p);
top(p()) main;

