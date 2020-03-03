#include <core.p4>

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
    bit<8>  val;
    myEnum1 val1;
    error   errorNoOne;
    H       h;
    bit<1>  z;
    bool    b;
    int<8>  i;
}

extern void f<D>(in D data);
struct tuple_0 {
    int<8>  field;
    bool    field_0;
    error   field_1;
    myEnum1 field_2;
    bit<8>  field_3;
    H       field_4;
    S       field_5;
}

control c(inout bit<1> r) {
    H s_2_h;
    H h_0;
    H x_0_field_4;
    H x_0_field_5_h;
    @hidden action convenient_init34() {
        s_2_h.setValid();
        s_2_h.i = 8s3;
        s_2_h.b = true;
        s_2_h.l = 14w0;
        s_2_h.b1 = 1w0;
        h_0.setValid();
        h_0.i = 8s0;
        h_0.b = false;
        h_0.l = 14w0;
        h_0.b1 = 1w0;
        s_2_h = h_0;
        x_0_field_4.i = 8s0;
        x_0_field_4.b = false;
        x_0_field_4.l = 14w0;
        x_0_field_4.b1 = 1w0;
        x_0_field_5_h.i = 8s0;
        x_0_field_5_h.b = false;
        x_0_field_5_h.l = 14w0;
        x_0_field_5_h.b1 = 1w0;
        f<tuple_0>({ 8s0, true, error.NoError, myEnum1.e1, 8w0, x_0_field_4, { 8w0, myEnum1.e1, error.NoError, x_0_field_5_h, 1w0, false, 8s0 } });
        r = 1w0;
    }
    @hidden table tbl_convenient_init34 {
        actions = {
            convenient_init34();
        }
        const default_action = convenient_init34();
    }
    apply {
        tbl_convenient_init34.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
