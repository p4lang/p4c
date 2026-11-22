#include <core.p4>

struct S {
    bit<32> b;
    bit<16> f;
}

struct G<T> {
    T b;
}

header H {
    bit<32> b;
    bit<16> f;
}

header I {
    bit<32> b;
}

header_union HU {
    H h;
    I i;
}

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

header H1 {
    int<8>  i;
    bool    b;
    bit<14> l;
    bit<1>  b1;
}

struct S1 {
    myEnum  val;
    myEnum1 val1;
    error   errorNoOne;
    H1      h;
    bit<1>  z;
    bool    b;
    int<8>  i;
}

extern void f<D>(in D data);
control C(out bit<32> x) {
    @name("C.stack") H[2] stack_0;
    @name("C.hustack") HU[2] hustack_0;
    @name("C.sa") S1 sa_0;
    @name("C.xx") tuple<int<8>, bool, error, myEnum1, myEnum, H1, S1> xx_0;
    apply {
        stack_0[0].setInvalid();
        stack_0[1].setInvalid();
        hustack_0[0].h.setInvalid();
        hustack_0[0].i.setInvalid();
        hustack_0[1].h.setInvalid();
        hustack_0[1].i.setInvalid();
        sa_0.h.setInvalid();
        x = 32w0;
        xx_0 = { 8s0, true, error.NoError, myEnum1.e1, (myEnum)8w0, (H1){#}, (S1){val = (myEnum)8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,h = (H1){#},z = 1w0,b = false,i = 8s0} };
        f<tuple<int<8>, bool, error, myEnum1, myEnum, H1, S1>>(xx_0);
    }
}

control CT(out bit<32> x);
package top(CT _c);
top(C()) main;
