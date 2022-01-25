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

S f<D>(in D data) {
    return { 1, ... };
}
control c(inout S r) {
    apply {
        S s = {b = true,h = {c = 1, ... }, ... };
        S sr = { ... };
        if (s == {b = false, ... }) {
            r.i = s.i;
        }
        tuple<bool, H, S> t;
        t = { ... };
        if (t != { ... }) {
            r.b = t[0];
        }
        f<S>({h = { 1, true, 2 }, ... });
        s = (s == { ... } ? s : sr);
        r.b = s.b;
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

