struct T {
    bit<1> f;
}

struct S {
    tuple<T, T> f1;
    T           f2;
    bit<1>      z;
}

extern void f<D>(in D data);
control c(inout bit<1> r) {
    S s;
    bit<1> tmp_0;
    apply {
        s = S {f1 = { T {f = 1w0}, T {f = 1w1} },f2 = T {f = 1w0},z = 1w1};
        f<tuple<T, T>>(s.f1);
        tmp_0 = s.f2.f & s.z;
        r = tmp_0;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

