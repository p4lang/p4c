struct T {
    bit<1> f;
}

struct tuple_0 {
    T f0;
    T f1;
}

struct S {
    tuple_0 f1;
    T       f2;
    bit<1>  z;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    T s_f1_f0;
    T s_f1_f1;
    @hidden action nestedtuple1l32() {
        s_f1_f0.f = 1w0;
        s_f1_f1.f = 1w1;
        f<tuple_0>((tuple_0){f0 = s_f1_f0,f1 = s_f1_f1});
        r = 1w0;
    }
    @hidden table tbl_nestedtuple1l32 {
        actions = {
            nestedtuple1l32();
        }
        const default_action = nestedtuple1l32();
    }
    apply {
        tbl_nestedtuple1l32.apply();
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
