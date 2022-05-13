#include <core.p4>

parser p(out bit<4> result) {
    @name("p.l_0") bit<4> l;
    @name("p.hasReturned") bool hasReturned;
    @name("p.retval") bit<4> retval;
    state start {
        l = 4w1;
        hasReturned = false;
        hasReturned = true;
        retval = l << 6w1;
        result = retval;
        transition accept;
    }
}

parser P(out bit<4> r);
package top(P p);
top(p()) main;

