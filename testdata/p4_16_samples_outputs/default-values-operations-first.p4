#include <core.p4>

header H {
    bit<1>  a;
    bool    b;
    bit<32> c;
}

struct S1 {
    bit<1> i;
    H      h;
    bool   b;
}

struct S {
    bit<32> a;
    bit<32> b;
}

S1 f(in S1 data) {
    bit<1> i = data.i;
    S1 returnTmp;
    returnTmp.i = i;
    returnTmp.b = false;
    return returnTmp;
}
control c(inout S1 r) {
    apply {
        S1 argTmp;
        argTmp.i = 1w1;
        argTmp.b = false;
        S leftTmp = (S){a = 32w11,b = 32w0};
        S rightTmp = (S){a = 32w11,b = 32w0};
        S leftTmp_0 = (S){a = 32w11,b = 32w0};
        S rightTmp_0 = (S){a = 32w11,b = 32w0};
        S rightTmp_1 = (S){a = 32w1,b = 32w0};
        S leftTmp_1 = (S){a = 32w1,b = 32w0};
        S rightTmp_2 = (S){a = 32w1,b = 32w0};
        S leftTmp_2 = (S){a = 32w1,b = 32w0};
        tuple<bit<32>, bit<32>> rightTmp_3 = { 32w1, 32w0 };
        tuple<bit<32>, bit<32>> leftTmp_3 = { 32w1, 32w0 };
        S1 val = f(argTmp);
        r.i = val.i;
        S s = (S){a = 32w11,b = 32w22};
        bool b1 = (S){a = 32w11,b = 32w11} == (S){a = 32w1,b = 32w2};
        bool b1_ = leftTmp == (S){a = 32w1,b = 32w1};
        bool b1__ = (S){a = 32w1,b = 32w1} == rightTmp;
        bool b2 = s == (S){a = 32w1,b = 32w2};
        bool b2_ = leftTmp_0 == s;
        bool b2__ = s == rightTmp_0;
        bool b3 = s == (S){a = 32w1,b = 32w2};
        bool b3_ = s == rightTmp_1;
        bool b3__ = leftTmp_1 == s;
        bool b4 = (S){a = 32w11,b = 32w22} == (S){a = 32w1,b = 32w2};
        bool b4_ = (S){a = 32w11,b = 32w22} == rightTmp_2;
        bool b4__ = leftTmp_2 == (S){a = 32w11,b = 32w22};
        tuple<bit<32>, bit<32>> t = { 32w1, 32w2 };
        bool b5 = t == { 32w1, 32w2 };
        bool b5_ = t == rightTmp_3;
        bool b5__ = leftTmp_3 == t;
    }
}

control simple(inout S1 r);
package top(simple e);
top(c()) main;

