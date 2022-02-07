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
    S returnTmp;
    returnTmp.i = 1w1;
    returnTmp.b = false;
    return returnTmp;
}
control c(inout S r) {
    apply {
        S argTmp = (S){i = 1w0,h = (H){a = 1w1,b = true,c = 32w2},b = false};
        S s = (S){i = 1w0,h = (H){a = 1w0,b = false,c = 32w1},b = true};
        S val = f(argTmp);
        r.b = s.b;
        r.i = val.i;
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

