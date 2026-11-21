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
    bit<8>  val;
    myEnum1 val1;
    error   errorNoOne;
    H1      h;
    bit<1>  z;
    bool    b;
    int<8>  i;
}

extern void f<D>(in D data);
struct tuple_0 {
    int<8>  f0;
    bool    f1;
    error   f2;
    myEnum1 f3;
    bit<8>  f4;
    H1      f5;
    S1      f6;
}

control C(out bit<32> x) {
    H1 ih;
    H1 ih_0;
    @name("C.stack") H[2] stack_0;
    @name("C.hustack") HU[2] hustack_0;
    H1 sa_0_h;
    @hidden action defaultinitializer61() {
        ih.setInvalid();
        ih_0.setInvalid();
        stack_0[0].setInvalid();
        stack_0[1].setInvalid();
        hustack_0[0].h.setInvalid();
        hustack_0[0].i.setInvalid();
        hustack_0[1].h.setInvalid();
        hustack_0[1].i.setInvalid();
        sa_0_h.setInvalid();
        x = 32w0;
        f<tuple_0>((tuple_0){f0 = 8s0,f1 = true,f2 = error.NoError,f3 = myEnum1.e1,f4 = 8w0,f5 = ih,f6 = (S1){val = 8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,h = ih_0,z = 1w0,b = false,i = 8s0}});
    }
    @hidden table tbl_defaultinitializer61 {
        actions = {
            defaultinitializer61();
        }
        const default_action = defaultinitializer61();
    }
    apply {
        tbl_defaultinitializer61.apply();
    }
}

control CT(out bit<32> x);
package top(CT _c);
top(C()) main;
