struct T {
    bit<1> f;
}

struct S {
    tuple<T, T>         f1;
    T                   f2;
    bit<1>              z;
    tuple<int<8>, bool> tpl;
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
    S s;
    H h;
    bit<1> tmp_0;
    tuple<int<8>, bool> x_0;
    H h_2;
    apply {
        x_0 = { 8s0, false };
        s = S {f1 = { T {f = 1w0}, T {f = 1w0} },f2 = T {f = 1w0},z = 1w0,tpl = { 8s0, false }};
        h_2.setValid();
        h_2 = H {i = 8s0,b = false,l = 7w0,l1 = 7w0,b1 = 1w0};
        h.setValid();
        h.b1 = h_2.b1;
        s.z = h.b1;
        f<tuple<T, T>>(s.f1);
        tmp_0 = s.f2.f & s.z;
        r = tmp_0;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
