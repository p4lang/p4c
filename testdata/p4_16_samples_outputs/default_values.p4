#include <core.p4>

enum bit<8> myEnum {
    One = 1,
    Two = 2,
    Three = 3
}

enum myEnum1 {
    e1,
    e2,
    e3
}

header H1 {
    bit<8>  bv_h1;
}

header H {
    int<8>  i;
    bool    b;
    bit<14>     bv_h;
    bit<1>  b_h;
}

header_union U {
    H1      h1;
    H       h2;
}

struct Recurs1 {
    H           h_rec;
    int<2>      i_rec;

}

struct Recurs {
    Recurs1     s_recurs;
    bool        b_recurs;
    bit<7>      bv_recurs;
    H           h_recurs;
}

struct S {
    tuple<int<8>, bool> tup_s;
    U       u_s;
    Recurs  strct_s;
    myEnum      val;
    myEnum1     val1;
    error       errorNoOne;
    H           h;
    bit<1>      z;
    bool        b;
    int<8>      i;
}


extern void f<D>(in D data);
control c(inout bit<1> r) {
    S sa;
    bit<1> tmp;
    apply {
        S sb;
        H br = {1, ...};
        sb = { b = true, h = {b_h = 1, ...}, ...};
        H hb = { ... };
        sa = sb;
        tuple<int<8>, bool,  H, H, U> x = { ... };
        f<tuple<int<8>, bool, H, H, U>>(x);
        f<S>(sa);
        tmp = sa.z;
        r = tmp;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
