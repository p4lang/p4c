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
    int<8> field;
    bool   field_0;
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
    int<8> field_1;
    bool   field_2;
    H      field_3;
    H      field_4;
    U      field_5;
}

control c(inout bit<1> r) {
    tuple_0 sa_0_tup_s;
    U sa_0_u_s;
    U sb_0_u_s;
    H sb_0_strct_s_s_recurs_h_rec;
    H sb_0_strct_s_h_recurs;
    H sb_0_h;
    U x_0_field_3;
    H hdr_or_hu;
    H hdr_or_hu_0;
    U hdr_or_hu_1;
    @hidden action default_values57() {
        sb_0_u_s.h1.setInvalid();
        sb_0_strct_s_s_recurs_h_rec.setInvalid();
        sb_0_strct_s_h_recurs.setInvalid();
        sb_0_h.setValid();
        sb_0_h.i = 8s0;
        sb_0_h.b = false;
        sb_0_h.bv_h = 14w0;
        sb_0_h.b_h = 1w1;
        sa_0_tup_s.field = 8s0;
        sa_0_tup_s.field_0 = false;
        sa_0_u_s.h1 = sb_0_u_s.h1;
        sa_0_u_s.h2 = sb_0_u_s.h2;
        hdr_or_hu.setInvalid();
        hdr_or_hu_0.setInvalid();
        hdr_or_hu_1.h1.setInvalid();
        x_0_field_3.h1 = hdr_or_hu_1.h1;
        x_0_field_3.h2 = hdr_or_hu_1.h2;
        f<tuple_1>({ 8s0, false, hdr_or_hu, hdr_or_hu_0, x_0_field_3 });
        f<S>({ sa_0_tup_s, sa_0_u_s, { { sb_0_strct_s_s_recurs_h_rec, 2s0 }, false, 7w0, sb_0_strct_s_h_recurs }, 8w0, myEnum1.e1, error.NoError, sb_0_h, 1w0, true, 8s0 });
        r = 1w0;
    }
    @hidden table tbl_default_values57 {
        actions = {
            default_values57();
        }
        const default_action = default_values57();
    }
    apply {
        tbl_default_values57.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
