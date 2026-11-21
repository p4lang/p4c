#include <core.p4>

const int cst = 0;
enum E {
    A,
    B,
    C
}

enum bit<32> Z {
    A = 32w2,
    B = 32w3
}

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
    H[2] stack;
    HU[2] hustack;
    S1 sa;
    bit<1> tmp;
    apply {
        x = 32w0;
        bool b = false;
        error e = error.NoError;
        E e1 = E.A;
        Z z = (Z)32w0;
        S s0 = (S){b = 32w0,f = 16w0};
        S s1 = (S){b = 32w0,f = 16w0};
        S s2 = (S){b = 32w2,f = 16w0};
        S s3 = (S){b = 32w2,f = 16w0};
        S s4 = (S){f = 16w2,b = 32w0};
        S s5 = (S){b = 32w2,f = 16w3};
        S s6 = (S){b = 32w0,f = 16w0};
        S s7 = (S){b = 32w2,f = 16w0};
        S s8 = (S){b = 32w2,f = 16w0};
        S s9 = (S){f = 16w2,b = 32w0};
        S s10 = (S){b = 32w2,f = 16w3};
        H h0 = (H){#};
        H h1 = (H){b = 32w0,f = 16w0};
        H h2 = (H){b = 32w2,f = 16w0};
        H h3 = (H){b = 32w2,f = 16w0};
        H h4 = (H){f = 16w2,b = 32w0};
        H h5 = (H){b = 32w2,f = 16w3};
        H h6 = (H){b = 32w0,f = 16w0};
        H h7 = (H){b = 32w2,f = 16w0};
        H h8 = (H){b = 32w2,f = 16w0};
        H h9 = (H){f = 16w2,b = 32w0};
        H h10 = (H){b = 32w2,f = 16w3};
        HU hu = (HU){#};
        stack = (H[2]){(H){#},(H){#}};
        stack = (H[2]){h0,(H){#}};
        stack = (H[2]){(H){#},(H){#}};
        hustack = (HU[2]){(HU){#},(HU){#}};
        hustack = (HU[2]){(HU){#},(HU){#}};
        hustack = (HU[2]){hu,(HU){#}};
        S1 sb = (S1){h = (H1){i = 8s3,b = true,l = 14w0,b1 = 1w0},b = false,val = (myEnum)8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,z = 1w0,i = 8s0};
        tuple<int<8>, bool, error, myEnum1, myEnum, H1, S1> xx = { 8s0, true, error.NoError, myEnum1.e1, (myEnum)8w0, (H1){#}, (S1){val = (myEnum)8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,h = (H1){#},z = 1w0,b = false,i = 8s0} };
        H1 hb = (H1){#};
        sb.h = hb;
        sa = sb;
        f<tuple<int<8>, bool, error, myEnum1, myEnum, H1, S1>>(xx);
        tmp = sa.z;
    }
}

control CT(out bit<32> x);
package top(CT _c);
top(C()) main;
