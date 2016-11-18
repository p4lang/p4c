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
    bit<1> tmp;
    apply {
        s = { { { 1w0 }, { 1w1 } }, { 1w0 }, 1w1 };
        f<tuple<T, T>>(s.f1);
        tmp = s.f2.f & s.z;
        tmp_0 = tmp;
        r = tmp_0;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;
