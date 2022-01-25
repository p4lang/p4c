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

header H1 {
    bit<8> bv_h1;
}

header H {
    int<8>  i;
    bool    b;
    bit<14> bv_h;
    bit<1>  b_h;
}

header_union U {
    H1 h1;
    H  h2;
}

struct Recurs1 {
    H      h_rec;
    int<2> i_rec;
}

struct Recurs {
    Recurs1 s_recurs;
    bool    b_recurs;
    bit<7>  bv_recurs;
    H       h_recurs;
}

struct S {
    tuple<int<8>, bool> tup_s;
    U                   u_s;
    Recurs              strct_s;
    myEnum              val;
    myEnum1             val1;
    error               errorNoOne;
    H                   h;
    bit<1>              z;
    bool                b;
    int<8>              i;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    S sa;
    bit<1> tmp;
    apply {
        S sb;
        H br = (H){i = 8s1,b = false,bv_h = 14w0,b_h = 1w0};
        sb.tup_s = { 8s0, false };
        sb.u_s.h1.setInvalid();
        sb.strct_s.s_recurs.h_rec.setInvalid();
        sb.strct_s.s_recurs.i_rec = 2s0;
        sb.strct_s.b_recurs = false;
        sb.strct_s.bv_recurs = 7w0;
        sb.strct_s.h_recurs.setInvalid();
        sb.val = (myEnum)8w0;
        sb.val1 = myEnum1.e1;
        sb.errorNoOne = error.NoError;
        sb.h = (H){i = 8s0,b = false,bv_h = 14w0,b_h = 1w1};
        sb.z = 1w0;
        sb.b = true;
        sb.i = 8s0;
        H hb;
        sa = sb;
        tuple<int<8>, bool, H, H, U> x;
        H hdr_or_hu;
        H hdr_or_hu_0;
        U hdr_or_hu_1;
        x = { 8s0, false, hdr_or_hu, hdr_or_hu_0, hdr_or_hu_1 };
        f<tuple<int<8>, bool, H, H, U>>(x);
        f<S>(sa);
        tmp = sa.z;
        r = tmp;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

