#include <core.p4>

header H {
    bit<1>  a;
    bool    b;
    bit<32> c;
}

struct S {
    bit<1> i;
    H      h;
    bool   b;
}

S f(in S data) {
    return { 1, ... };
}
control c(inout S r) {
    apply {
        S s = {b = true,h = {c = 1, ... }, ... };
        S val = f({h = { 1, true, 2 }, ... });
        r.b = s.b;
        r.i = val.i;
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;
