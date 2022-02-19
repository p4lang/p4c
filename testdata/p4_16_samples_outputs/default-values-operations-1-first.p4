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

S f_0(in S data) {
    S returnTmp;
    returnTmp.i = 1w1;
    returnTmp.b = false;
    return returnTmp;
}
S f<D>(in D data) {
    S returnTmp;
    returnTmp.i = 1w1;
    returnTmp.b = false;
    return returnTmp;
}
control c(inout S r) {
    apply {
        S rightTmp;
        rightTmp.i = 1w0;
        rightTmp.b = false;
        tuple<bool, H, S> rightTmp_0;
        H hdr_or_hu;
        H hdr_or_hu_0;
        rightTmp_0 = { false, hdr_or_hu, (S){i = 1w0,h = hdr_or_hu_0,b = false} };
        S argTmp = (S){i = 1w0,h = (H){a = 1w1,b = true,c = 32w2},b = false};
        S leftTmp;
        leftTmp.i = 1w0;
        leftTmp.b = true;
        S s = (S){i = 1w0,h = (H){a = 1w0,b = false,c = 32w1},b = true};
        S sr;
        sr.i = 1w0;
        sr.b = false;
        if (s == rightTmp) {
            r.i = s.i;
        }
        tuple<bool, H, S> t;
        {
            H hdr_or_hu_1;
            hdr_or_hu_1.setInvalid();
            H hdr_or_hu_2;
            hdr_or_hu_2.setInvalid();
            t = { false, hdr_or_hu_1, (S){i = 1w0,h = hdr_or_hu_2,b = false} };
        }
        if (t != rightTmp_0) {
            r.b = t[0];
        }
        f_0(argTmp);
        s = (leftTmp == s ? s : sr);
        r.b = s.b;
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

