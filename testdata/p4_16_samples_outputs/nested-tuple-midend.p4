struct T {
    bit<1> f;
}

struct tuple_1 {
    T field_1;
    T field_2;
}

struct S {
    tuple_1 f1;
    T       f2;
    bit<1>  z;
}

struct tuple_0 {
    T field;
    T field_0;
}

extern void f<T>(in T data);
control c(inout bit<1> r) {
    T s_0_f1_field;
    T s_0_f1_field_0;
    @hidden action nestedtuple34() {
        s_0_f1_field.f = 1w0;
        s_0_f1_field_0.f = 1w1;
        f<tuple_1>({ s_0_f1_field, s_0_f1_field_0 });
        f<tuple_0>((tuple_0){field = (T){f = 1w0},field_0 = (T){f = 1w1}});
        r = 1w0;
    }
    @hidden table tbl_nestedtuple34 {
        actions = {
            nestedtuple34();
        }
        const default_action = nestedtuple34();
    }
    apply {
        tbl_nestedtuple34.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

