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
    S s_0;
    H h_0;
    bit<1> tmp;
    apply {
        S s_1;
        s_1 = S {b = false,h = { 3, true, ... }, ...};
        H h = { ... };
        s_1.h = h;
        s_0 = s_1;
        tuple<int<8>, bool, error, myEnum1, myEnum, H, S> x = { 0, true, ... };
        f<tuple<int<8>, bool, error, myEnum1, myEnum, H, S>>(x);
        tmp = s_0.z;
        r = tmp;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
