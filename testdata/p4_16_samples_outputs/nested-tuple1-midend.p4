struct T {
    bit<1> f;
}

struct tuple_0 {
    T field;
    T field_0;
}

struct S {
    tuple_0 f1;
    T       f2;
    bit<1>  z;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    T s_f1_field;
    T s_f1_field_0;
    @hidden action act() {
        s_f1_field.f = 1w0;
        s_f1_field_0.f = 1w1;
        f<tuple_0>({ s_f1_field, s_f1_field_0 });
        r = 1w0;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

