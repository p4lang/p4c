#include <core.p4>

enum bit<8> myEnum {
    One = 8w1,
    Two = 8w2,
    Three = 8w3
}

enum myEnum1 {
    e1,
    e2,
    e3
}

header H {
    int<8>  i;
    bool    b;
    bit<14> l;
    bit<1>  b1;
}

struct S {
    myEnum  val;
    myEnum1 val1;
    error   errorNoOne;
    H       h;
    bit<1>  z;
    bool    b;
    int<8>  i;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    S s;
    bit<1> tmp_0;
    S s_2;
    H h_0;
    tuple<int<8>, bool, error, myEnum1, myEnum, H, S> x_0;
    apply {
        s_2.h.setValid();
        s_2 = S {val = 8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,h = H {i = 8s3,b = true,l = 14w0,b1 = 1w0},z = 1w0,b = false,i = 8s0};
        h_0.setValid();
        h_0 = H {i = 8s0,b = false,l = 14w0,b1 = 1w0};
        s_2.h = h_0;
        s = s_2;
        x_0 = { 8s0, true, error.NoError, myEnum1.e1, 8w0, H {i = 8s0,b = false,l = 14w0,b1 = 1w0}, S {val = 8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,h = H {i = 8s0,b = false,l = 14w0,b1 = 1w0},z = 1w0,b = false,i = 8s0} };
        f<tuple<int<8>, bool, error, myEnum1, myEnum, H, S>>(x_0);
        tmp_0 = s.z;
        r = tmp_0;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
