#include <core.p4>

parser p(out bit<4> result) {
    @name("p.l_0") bit<4> l;
    @name("p.retval") bit<4> retval;
    @name("p.inlinedRetval") bit<4> inlinedRetval_0;
    state start {
        l = 4w1;
        retval = l << 6w1;
        inlinedRetval_0 = retval;
        result = inlinedRetval_0;
        transition accept;
    }
}

parser P(out bit<4> r);
package top(P p);
top(p()) main;
