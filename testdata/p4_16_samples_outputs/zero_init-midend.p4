struct T {
    bit<1> f;
}

struct tuple_0 {
    T field;
    T field_0;
}

struct tuple_1 {
    int<8> field_1;
    bool   field_2;
}

struct S {
    tuple_0 f1;
    T       f2;
    bit<1>  z;
    tuple_1 tpl;
}

header H {
    int<8> i;
    bool   b;
    bit<7> l;
    bit<7> l1;
    bit<1> b1;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    T s_f1_field;
    T s_f1_field_0;
    H h;
    H h_2;
    @hidden action zero_init28() {
        s_f1_field.f = 1w0;
        s_f1_field_0.f = 1w0;
        h_2.setValid();
        h_2.i = 8s0;
        h_2.b = false;
        h_2.l = 7w0;
        h_2.l1 = 7w0;
        h_2.b1 = 1w0;
        h.setValid();
        h.b1 = 1w0;
        f<tuple_0>({ s_f1_field, s_f1_field_0 });
        r = 1w0;
    }
    @hidden table tbl_zero_init28 {
        actions = {
            zero_init28();
        }
        const default_action = zero_init28();
    }
    apply {
        tbl_zero_init28.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
