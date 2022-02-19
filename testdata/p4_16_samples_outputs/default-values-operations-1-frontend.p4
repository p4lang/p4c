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

control c(inout S r) {
    @name("c.rightTmp") S rightTmp_1;
    @name("c.rightTmp_0") tuple<bool, H, S> rightTmp_2;
    @name("c.hdr_or_hu") H hdr_or_hu_3;
    @name("c.hdr_or_hu_0") H hdr_or_hu_4;
    @name("c.leftTmp") S leftTmp_0;
    @name("c.s") S s_0;
    @name("c.sr") S sr_0;
    @name("c.t") tuple<bool, H, S> t_0;
    @name("c.hdr_or_hu_1") H hdr_or_hu_5;
    @name("c.hdr_or_hu_2") H hdr_or_hu_6;
    @name("c.tmp") S tmp;
    apply {
        rightTmp_1.h.setInvalid();
        rightTmp_1.i = 1w0;
        rightTmp_1.b = false;
        hdr_or_hu_3.setInvalid();
        hdr_or_hu_4.setInvalid();
        rightTmp_2 = { false, hdr_or_hu_3, (S){i = 1w0,h = hdr_or_hu_4,b = false} };
        leftTmp_0.h.setInvalid();
        leftTmp_0.i = 1w0;
        leftTmp_0.b = true;
        s_0.h.setValid();
        s_0 = (S){i = 1w0,h = (H){a = 1w0,b = false,c = 32w1},b = true};
        sr_0.h.setInvalid();
        sr_0.i = 1w0;
        sr_0.b = false;
        if (s_0 == rightTmp_1) {
            r.i = s_0.i;
        }
        hdr_or_hu_5.setInvalid();
        hdr_or_hu_5.setInvalid();
        hdr_or_hu_6.setInvalid();
        hdr_or_hu_6.setInvalid();
        t_0 = { false, hdr_or_hu_5, (S){i = 1w0,h = hdr_or_hu_6,b = false} };
        if (leftTmp_0 == s_0) {
            tmp = s_0;
        } else {
            tmp = sr_0;
        }
        s_0 = tmp;
        r.b = s_0.b;
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

