#include <core.p4>

parser p(out bit<4> result) {
    state start {
        result = 4w2;
        transition accept;
    }
}

parser P(out bit<4> r);
package top(P p);
top(p()) main;

