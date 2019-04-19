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
    S s_0;
    bit<1> tmp;
    apply {
        s_0 = {{ {1w0}, {1w1} },{1w0},1w1};
        f<tuple<T, T>>(s_0.f1);
        tmp = s_0.f2.f & s_0.z;
        r = tmp;
    }
}

control simple(inout bit<1> r);
package top(simple e);
top(c()) main;

