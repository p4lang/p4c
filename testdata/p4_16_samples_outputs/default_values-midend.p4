#include <core.p4>

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

struct tuple_0 {
    int<8> f0;
    bool   f1;
}

struct S {
    tuple_0 tup_s;
    U       u_s;
    Recurs  strct_s;
    bit<8>  val;
    myEnum1 val1;
    error   errorNoOne;
    H       h;
    bit<1>  z;
    bool    b;
    int<8>  i;
}

extern void f<D>(in D data);
struct tuple_1 {
    int<8> f0;
    bool   f1;
    H      f2;
    H      f3;
    U      f4;
}

control c(inout bit<1> r) {
    tuple_0 sa_0_tup_s;
    U sa_0_u_s;
    H sa_0_strct_s_s_recurs_h_rec;
    H sa_0_strct_s_h_recurs;
    H sa_0_h;
    U sb_0_u_s;
    H sb_0_strct_s_s_recurs_h_rec;
    H sb_0_strct_s_h_recurs;
    H sb_0_h;
    U x_0_f4;
    @name("c.hdr_or_hu") H hdr_or_hu_2;
    @name("c.hdr_or_hu_0") H hdr_or_hu_3;
    @name("c.hdr_or_hu_1") U hdr_or_hu_4;
    @hidden action default_values60() {
        sa_0_u_s.h1.setInvalid();
        sa_0_u_s.h2.setInvalid();
        sa_0_strct_s_s_recurs_h_rec.setInvalid();
        sa_0_strct_s_h_recurs.setInvalid();
        sa_0_h.setInvalid();
        sb_0_u_s.h1.setInvalid();
        sb_0_u_s.h2.setInvalid();
        sb_0_strct_s_s_recurs_h_rec.setInvalid();
        sb_0_strct_s_h_recurs.setInvalid();
        sb_0_h.setInvalid();
        sb_0_u_s.h1.setInvalid();
        sb_0_strct_s_s_recurs_h_rec.setInvalid();
        sb_0_strct_s_h_recurs.setInvalid();
        sb_0_h.setValid();
        sb_0_h.i = 8s0;
        sb_0_h.b = false;
        sb_0_h.bv_h = 14w0;
        sb_0_h.b_h = 1w1;
        sa_0_tup_s.f0 = 8s0;
        sa_0_tup_s.f1 = false;
        sa_0_u_s.h1 = sb_0_u_s.h1;
        sa_0_u_s.h2 = sb_0_u_s.h2;
        sa_0_strct_s_s_recurs_h_rec = sb_0_strct_s_s_recurs_h_rec;
        sa_0_strct_s_h_recurs = sb_0_strct_s_h_recurs;
        sa_0_h = sb_0_h;
        hdr_or_hu_2.setInvalid();
        hdr_or_hu_3.setInvalid();
        hdr_or_hu_4.h1.setInvalid();
        hdr_or_hu_4.h2.setInvalid();
        x_0_f4.h1 = hdr_or_hu_4.h1;
        x_0_f4.h2 = hdr_or_hu_4.h2;
        f<tuple_1>((tuple_1){f0 = 8s0,f1 = false,f2 = hdr_or_hu_2,f3 = hdr_or_hu_3,f4 = x_0_f4});
        f<S>((S){tup_s = sa_0_tup_s,u_s = sa_0_u_s,strct_s = (Recurs){s_recurs = (Recurs1){h_rec = sb_0_strct_s_s_recurs_h_rec,i_rec = 2s0},b_recurs = false,bv_recurs = 7w0,h_recurs = sb_0_strct_s_h_recurs},val = 8w0,val1 = myEnum1.e1,errorNoOne = error.NoError,h = sb_0_h,z = 1w0,b = true,i = 8s0});
        r = 1w0;
    }
    @hidden table tbl_default_values60 {
        actions = {
            default_values60();
        }
        const default_action = default_values60();
    }
    apply {
        tbl_default_values60.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

